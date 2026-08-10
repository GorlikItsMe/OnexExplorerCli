#include <onex/archive/effect/effect_definition.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>

namespace onex::archive {

  namespace {

    constexpr int kRootHeaderSize = 24;
    constexpr int kComponentRecordSize = 192;
    constexpr int kTrackDescriptorBase = 0x60;
    constexpr int kTrackDescriptorSize = 12;
    constexpr int kTrackCount = 8;

    // Kind byte of a component record selects the kind-specific union.
    constexpr uint8_t kKindPositioned = 0;
    constexpr uint8_t kKindFlying = 1;
    constexpr uint8_t kKindParticle = 2;

    enum class TrackType { kQuaternion, kVector3, kColor, kTexture };

    struct TrackInfo {
      const char* name;  // JSON key of the sub-track within its group
      TrackType type;
      size_t value_size;
      bool in_texture_transform_group;  // false → transform group (or top level)
    };

    // The eight track descriptors of a component, in descriptor order.
    constexpr std::array<TrackInfo, kTrackCount> kTracks = {{
        {"rotation", TrackType::kQuaternion, 16, false},
        {"scale", TrackType::kVector3, 12, false},
        {"translation", TrackType::kVector3, 12, false},
        {"color", TrackType::kColor, 4, false},
        {"texture", TrackType::kTexture, 4, false},
        {"rotation", TrackType::kQuaternion, 16, true},
        {"scale", TrackType::kVector3, 12, true},
        {"translation", TrackType::kVector3, 12, true},
    }};

    // A cursor-based reader with bounds checking. All reads fail gracefully.
    struct Reader {
      std::span<const uint8_t> data;
      size_t pos = 0;

      bool bytes(size_t count) const { return pos + count <= data.size(); }
      bool u8(uint8_t& out) {
        if (!bytes(1)) return false;
        out = data[pos++];
        return true;
      }
      bool i16(int16_t& out) {
        if (!bytes(2)) return false;
        out = static_cast<int16_t>(static_cast<uint16_t>(data[pos])
                                   | static_cast<uint16_t>(data[pos + 1]) << 8);
        pos += 2;
        return true;
      }
      bool u16(uint16_t& out) {
        if (!bytes(2)) return false;
        out = static_cast<uint16_t>(data[pos]) | static_cast<uint16_t>(data[pos + 1]) << 8;
        pos += 2;
        return true;
      }
      bool i32(int32_t& out) {
        if (!bytes(4)) return false;
        out = static_cast<int32_t>(static_cast<uint32_t>(data[pos])
                                   | static_cast<uint32_t>(data[pos + 1]) << 8
                                   | static_cast<uint32_t>(data[pos + 2]) << 16
                                   | static_cast<uint32_t>(data[pos + 3]) << 24);
        pos += 4;
        return true;
      }
      bool f32(float& out) {
        int32_t raw;
        if (!i32(raw)) return false;
        std::memcpy(&out, &raw, sizeof(raw));
        return true;
      }
    };

    // Offset-based reads into a fixed-size record, checked against a limit.
    auto u8_at(const uint8_t* rec, size_t rec_size, size_t off, uint8_t& out) -> bool {
      if (off + 1 > rec_size) return false;
      out = rec[off];
      return true;
    }
    auto i32_at(const uint8_t* rec, size_t rec_size, size_t off, int32_t& out) -> bool {
      if (off + 4 > rec_size) return false;
      out = static_cast<int32_t>(
          static_cast<uint32_t>(rec[off]) | static_cast<uint32_t>(rec[off + 1]) << 8
          | static_cast<uint32_t>(rec[off + 2]) << 16 | static_cast<uint32_t>(rec[off + 3]) << 24);
      return true;
    }
    auto u16_at(const uint8_t* rec, size_t rec_size, size_t off, uint16_t& out) -> bool {
      if (off + 2 > rec_size) return false;
      out = static_cast<uint16_t>(rec[off]) | static_cast<uint16_t>(rec[off + 1]) << 8;
      return true;
    }
    auto f32_at(const uint8_t* rec, size_t rec_size, size_t off, float& out) -> bool {
      int32_t raw;
      if (!i32_at(rec, rec_size, off, raw)) return false;
      std::memcpy(&out, &raw, sizeof(raw));
      return true;
    }

    // Packed 16-bit float: sign(1) + exponent(4, bias 7) + mantissa(11).
    // Mirrors taletool PackedFloat16::to_f32.
    auto packed_f16(uint16_t v) -> float {
      if (v == 0) {
        return 0.0f;
      }
      const uint32_t sign = (v & 0x8000) >> 15;
      const uint32_t exponent = ((v & 0x7800) >> 11) - 7 + 127;
      const uint32_t mantissa = v & 0x07FF;
      const uint32_t bits = (sign << 31) | (exponent << 23) | (mantissa << 12);
      float out;
      std::memcpy(&out, &bits, sizeof(bits));
      return out;
    }

    auto json_vector3(const uint8_t* rec, size_t rec_size, size_t off)
        -> std::optional<nlohmann::json> {
      float x, y, z;
      if (!f32_at(rec, rec_size, off, x) || !f32_at(rec, rec_size, off + 4, y)
          || !f32_at(rec, rec_size, off + 8, z)) {
        return std::nullopt;
      }
      return nlohmann::json::array({x, y, z});
    }

    // The kind-specific union of a component record.
    auto decode_variant(const uint8_t* rec, size_t rec_size, uint8_t kind)
        -> std::optional<nlohmann::json> {
      switch (kind) {
        case kKindPositioned: {
          uint8_t placement, update_source, use_target;
          float source_scale, target_scale;
          if (!u8_at(rec, rec_size, 0x18, placement) || !f32_at(rec, rec_size, 0x19, source_scale)
              || !u8_at(rec, rec_size, 0x1D, update_source)
              || !u8_at(rec, rec_size, 0x1E, use_target)
              || !f32_at(rec, rec_size, 0x1F, target_scale)) {
            return std::nullopt;
          }
          return nlohmann::json{
              {"kind", "positioned"},
              {"placement_mode", placement},
              {"source_height_scale", source_scale},
              {"update_source_height", update_source},
              {"use_target_height", use_target},
              {"target_height_scale", target_scale},
          };
        }
        case kKindFlying: {
          uint8_t placement, rotate_sine_offset;
          float target_scale, source_scale, fade_out, travel, sine_height;
          uint16_t sine_offset_x_raw;
          auto target_offset = json_vector3(rec, rec_size, 0x1D);
          auto source_offset = json_vector3(rec, rec_size, 0x2D);
          if (!u8_at(rec, rec_size, 0x18, placement) || !f32_at(rec, rec_size, 0x19, target_scale)
              || !target_offset || !f32_at(rec, rec_size, 0x29, source_scale) || !source_offset
              || !f32_at(rec, rec_size, 0x39, fade_out) || !f32_at(rec, rec_size, 0x3D, travel)
              || !f32_at(rec, rec_size, 0x41, sine_height)
              || !u8_at(rec, rec_size, 0x45, rotate_sine_offset)
              || !u16_at(rec, rec_size, 0x46, sine_offset_x_raw)) {
            return std::nullopt;
          }
          return nlohmann::json{
              {"kind", "flying"},
              {"placement_mode", placement},
              {"target_height_scale", target_scale},
              {"target_offset", *target_offset},
              {"source_height_scale", source_scale},
              {"source_offset", *source_offset},
              {"fade_out_distance", fade_out},
              {"travel_rate", travel},
              {"sine_height_scale", sine_height},
              {"rotate_sine_offset", rotate_sine_offset},
              {"sine_offset_x", packed_f16(sine_offset_x_raw)},
          };
        }
        case kKindParticle: {
          uint8_t flags, gravity, initial_spawn, spawn_base, spawn_random;
          uint16_t spawn_delay_base, spawn_delay_random, lifetime_base, lifetime_random;
          uint16_t spawn_raw[3];
          uint16_t rotation_raw[4];
          uint16_t range_raw[3], size_raw[3];
          uint8_t axis_rand[3], rotation_rand[3], scale_rand[3];
          if (!u8_at(rec, rec_size, 0x18, flags) || !u16_at(rec, rec_size, 0x19, spawn_raw[0])
              || !u16_at(rec, rec_size, 0x1B, spawn_raw[1])
              || !u16_at(rec, rec_size, 0x1D, spawn_raw[2])) {
            return std::nullopt;
          }
          for (int i = 0; i < 4; ++i) {
            if (!u16_at(rec, rec_size, 0x1F + 2 * i, rotation_raw[i])) return std::nullopt;
          }
          for (int i = 0; i < 3; ++i) {
            if (!u16_at(rec, rec_size, 0x27 + 2 * i, range_raw[i])) return std::nullopt;
          }
          for (int i = 0; i < 3; ++i) {
            if (!u16_at(rec, rec_size, 0x2D + 2 * i, size_raw[i])) return std::nullopt;
          }
          for (int i = 0; i < 3; ++i) {
            if (!u8_at(rec, rec_size, 0x33 + i, axis_rand[i])) return std::nullopt;
          }
          for (int i = 0; i < 3; ++i) {
            if (!u8_at(rec, rec_size, 0x36 + i, rotation_rand[i])) return std::nullopt;
          }
          for (int i = 0; i < 3; ++i) {
            if (!u8_at(rec, rec_size, 0x39 + i, scale_rand[i])) return std::nullopt;
          }
          if (!u8_at(rec, rec_size, 0x3C, gravity) || !u8_at(rec, rec_size, 0x3D, initial_spawn)
              || !u8_at(rec, rec_size, 0x3E, spawn_base)
              || !u8_at(rec, rec_size, 0x3F, spawn_random)
              || !u16_at(rec, rec_size, 0x40, spawn_delay_base)
              || !u16_at(rec, rec_size, 0x42, spawn_delay_random)
              || !u16_at(rec, rec_size, 0x44, lifetime_base)
              || !u16_at(rec, rec_size, 0x46, lifetime_random)) {
            return std::nullopt;
          }
          return nlohmann::json{
              {"kind", "particle"},
              {"flags", flags},
              {"spawn_offset",
               nlohmann::json::array(
                   {packed_f16(spawn_raw[0]), packed_f16(spawn_raw[1]), packed_f16(spawn_raw[2])})},
              {"rotation",
               nlohmann::json::array(
                   {static_cast<int16_t>(rotation_raw[0]), static_cast<int16_t>(rotation_raw[1]),
                    static_cast<int16_t>(rotation_raw[2]), static_cast<int16_t>(rotation_raw[3])})},
              {"axis_random_range",
               nlohmann::json::array(
                   {packed_f16(range_raw[0]), packed_f16(range_raw[1]), packed_f16(range_raw[2])})},
              {"particle_size",
               nlohmann::json::array(
                   {packed_f16(size_raw[0]), packed_f16(size_raw[1]), packed_f16(size_raw[2])})},
              {"axis_randomization",
               nlohmann::json::array({axis_rand[0], axis_rand[1], axis_rand[2]})},
              {"rotation_randomization",
               nlohmann::json::array({rotation_rand[0], rotation_rand[1], rotation_rand[2]})},
              {"scale_randomization",
               nlohmann::json::array({scale_rand[0], scale_rand[1], scale_rand[2]})},
              {"gravity_factor", gravity},
              {"initial_spawn_count", initial_spawn},
              {"spawn_count_base", spawn_base},
              {"spawn_count_random_range", spawn_random},
              {"spawn_delay_base_ms", spawn_delay_base},
              {"spawn_delay_random_range_ms", spawn_delay_random},
              {"particle_lifetime_base_ms", lifetime_base},
              {"particle_lifetime_random_range_ms", lifetime_random},
          };
        }
        default:
          return nlohmann::json{{"kind", kind}};
      }
    }

    // One component: fixed properties record + variable track data.
    auto decode_component(Reader& body, const uint8_t* rec) -> std::optional<nlohmann::json> {
      constexpr size_t kRecSize = kComponentRecordSize;
      uint8_t orientation_mode, timeline_mode, record_timeline, kind;
      int32_t source_blend, destination_blend, geometry_key, callback;
      uint16_t first_key, last_key, rate, units_per_key;
      int32_t start_offset, duration_or_loop;
      uint8_t loop_animation, cursor_mode, timing_mode, enable_depth, transform_enabled, base_scale,
          texture_transform_enabled;
      if (!u8_at(rec, kRecSize, 5, orientation_mode) || !u8_at(rec, kRecSize, 6, timeline_mode)
          || !u8_at(rec, kRecSize, 7, record_timeline) || !u8_at(rec, kRecSize, 4, kind)
          || !i32_at(rec, kRecSize, 0x08, source_blend)
          || !i32_at(rec, kRecSize, 0x0C, destination_blend)
          || !i32_at(rec, kRecSize, 0x10, geometry_key) || !i32_at(rec, kRecSize, 0x14, callback)
          || !u16_at(rec, kRecSize, 0x48, first_key) || !u16_at(rec, kRecSize, 0x4A, last_key)
          || !u16_at(rec, kRecSize, 0x4C, rate) || !u16_at(rec, kRecSize, 0x4E, units_per_key)
          || !i32_at(rec, kRecSize, 0x50, start_offset)
          || !i32_at(rec, kRecSize, 0x54, duration_or_loop)
          || !u8_at(rec, kRecSize, 0x58, loop_animation) || !u8_at(rec, kRecSize, 0x59, cursor_mode)
          || !u8_at(rec, kRecSize, 0x5A, timing_mode) || !u8_at(rec, kRecSize, 0x5B, enable_depth)
          || !u8_at(rec, kRecSize, 0x5C, transform_enabled)
          || !u8_at(rec, kRecSize, 0x5D, base_scale)
          || !u8_at(rec, kRecSize, 0x5F, texture_transform_enabled)) {
        return std::nullopt;
      }

      auto variant = decode_variant(rec, kRecSize, kind);
      if (!variant) {
        return std::nullopt;
      }

      // Track descriptors: one 12-byte slot per track, key count in byte 0.
      int key_counts[kTrackCount];
      for (int t = 0; t < kTrackCount; ++t) {
        uint8_t count;
        if (!u8_at(rec, kRecSize, kTrackDescriptorBase + t * kTrackDescriptorSize, count)) {
          return std::nullopt;
        }
        key_counts[t] = count;
      }

      // Track keyframes. Within one track the data is laid out as all u16 key
      // times first, followed by the parallel value array.
      nlohmann::json transform = nlohmann::json::object();
      nlohmann::json texture_transform = nlohmann::json::object();
      nlohmann::json color = nlohmann::json::array();
      nlohmann::json texture = nlohmann::json::array();
      for (int t = 0; t < kTrackCount; ++t) {
        const int count = key_counts[t];
        if (count == 0) {
          continue;
        }
        uint16_t times[256];
        for (int k = 0; k < count; ++k) {
          if (!body.u16(times[k])) {
            return std::nullopt;
          }
        }
        nlohmann::json keys = nlohmann::json::array();
        for (int k = 0; k < count; ++k) {
          switch (kTracks[t].type) {
            case TrackType::kQuaternion: {
              float x, y, z, w;
              if (!body.f32(x) || !body.f32(y) || !body.f32(z) || !body.f32(w)) {
                return std::nullopt;
              }
              keys.push_back(nlohmann::json{{"time", times[k]},
                                            {"value", nlohmann::json::array({x, y, z, w})}});
              break;
            }
            case TrackType::kVector3: {
              float x, y, z;
              if (!body.f32(x) || !body.f32(y) || !body.f32(z)) {
                return std::nullopt;
              }
              keys.push_back(
                  nlohmann::json{{"time", times[k]}, {"value", nlohmann::json::array({x, y, z})}});
              break;
            }
            case TrackType::kColor: {
              uint8_t blue, green, red, alpha;
              if (!body.u8(blue) || !body.u8(green) || !body.u8(red) || !body.u8(alpha)) {
                return std::nullopt;
              }
              keys.push_back(nlohmann::json{
                  {"time", times[k]},
                  {"color",
                   nlohmann::json{
                       {"red", red}, {"green", green}, {"blue", blue}, {"alpha", alpha}}},
              });
              break;
            }
            case TrackType::kTexture: {
              int32_t resource_key;
              if (!body.i32(resource_key)) {
                return std::nullopt;
              }
              keys.push_back(
                  nlohmann::json{{"time", times[k]}, {"texture_resource_key", resource_key}});
              break;
            }
          }
        }
        if (t == 3) {
          color = std::move(keys);
        } else if (t == 4) {
          texture = std::move(keys);
        } else if (kTracks[t].in_texture_transform_group) {
          texture_transform[kTracks[t].name] = std::move(keys);
        } else {
          transform[kTracks[t].name] = std::move(keys);
        }
      }

      nlohmann::json properties = {
          {"orientation_mode", orientation_mode},
          {"timeline_mode", timeline_mode},
          {"record_timeline_value", record_timeline},
          {"source_blend_factor", source_blend},
          {"destination_blend_factor", destination_blend},
          {"geometry_resource_key", geometry_key},
          {"callback_value", callback},
          {"animation_timing", nlohmann::json{{"first_key", first_key},
                                              {"last_key", last_key},
                                              {"rate", rate},
                                              {"units_per_key", units_per_key}}},
          {"start_offset_ms", start_offset},
          {"duration_or_loop_ms", duration_or_loop},
          {"loop_animation", loop_animation},
          {"cursor_mode", cursor_mode},
          {"timing_mode", timing_mode},
          {"enable_depth_state", enable_depth},
          {"transform_animation_enabled", transform_enabled},
          {"base_scale", base_scale},
          {"texture_transform_animation_enabled", texture_transform_enabled},
          {"variant", *variant},
      };

      for (const auto& sub : {"rotation", "scale", "translation"}) {
        if (!transform.contains(sub)) {
          transform[sub] = nlohmann::json::array();
        }
        if (!texture_transform.contains(sub)) {
          texture_transform[sub] = nlohmann::json::array();
        }
      }

      return nlohmann::json{
          {"properties", std::move(properties)},
          {"transform", std::move(transform)},
          {"color", std::move(color)},
          {"texture", std::move(texture)},
          {"texture_transform", std::move(texture_transform)},
      };
    }

  }  // namespace

  auto decode_effect_definition(std::span<const uint8_t> data) -> Result<nlohmann::json> {
    if (data.size() < kRootHeaderSize) {
      return {{}, Error::kInvalidFormat};
    }

    const int32_t resource_key = static_cast<int32_t>(
        static_cast<uint32_t>(data[0]) | static_cast<uint32_t>(data[1]) << 8
        | static_cast<uint32_t>(data[2]) << 16 | static_cast<uint32_t>(data[3]) << 24);
    const uint16_t component_count
        = static_cast<uint16_t>(data[0x0C]) | static_cast<uint16_t>(data[0x0D]) << 8;

    nlohmann::json loader_workspace;
    {
      const auto slot = [&data](size_t off) -> int32_t {
        return static_cast<int32_t>(static_cast<uint32_t>(data[off])
                                    | static_cast<uint32_t>(data[off + 1]) << 8
                                    | static_cast<uint32_t>(data[off + 2]) << 16
                                    | static_cast<uint32_t>(data[off + 3]) << 24);
      };
      loader_workspace = {
          {"loaded_tick_slot", slot(0x04)},
          {"source_record_slot", slot(0x08)},
          {"child_records_slot", slot(0x0E)},
          {"reference_count_slot", slot(0x12)},
          {"flags_slot",
           static_cast<uint16_t>(data[0x16]) | static_cast<uint16_t>(data[0x17]) << 8},
      };
    }

    // Fixed component records follow the root header.
    const size_t fixed_size
        = kRootHeaderSize + static_cast<size_t>(component_count) * kComponentRecordSize;
    if (data.size() < fixed_size) {
      return {{}, Error::kInvalidFormat};
    }

    Reader body{data, fixed_size};
    nlohmann::json components = nlohmann::json::array();
    for (int c = 0; c < component_count; ++c) {
      const uint8_t* rec = data.data() + kRootHeaderSize + c * kComponentRecordSize;
      auto component = decode_component(body, rec);
      if (!component) {
        return {{}, Error::kInvalidFormat};
      }
      components.push_back(std::move(*component));
    }

    return {nlohmann::json{
        {"resource_key", resource_key},
        {"loader_workspace", std::move(loader_workspace)},
        {"components", std::move(components)},
    }};
  }

}  // namespace onex::archive
