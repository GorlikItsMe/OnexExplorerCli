#include <onex/archive/sprite/sprite_info.h>

#include <array>
#include <nlohmann/json.hpp>
#include <string>

namespace onex::archive {

  namespace {

    // 16-byte header: direction(1), animation(1), monster(2) or sex/class(1) +
    // morph(1), base(4), nspm(4), kit(4). Then 7 counted texture-part lists,
    // each `count(1)` x `{index(2), id(4)}`.
    constexpr size_t kHeaderSize = 16;
    constexpr size_t kPartCount = 7;
    constexpr size_t kCellSize = 6;

    // Player part list names, in list order.
    constexpr std::array<const char*, kPartCount> kPlayerPartNames = {
        "armor", "hair", "hat", "mask", "secondary-Weapon", "first-Weapon", "weapon-part",
    };

    auto read_u16_le(const uint8_t* p) -> uint16_t {
      return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8;
    }

    auto read_u32_le(const uint8_t* p) -> uint32_t {
      return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8
             | static_cast<uint32_t>(p[2]) << 16 | static_cast<uint32_t>(p[3]) << 24;
    }

  }  // namespace

  auto decode_sprite_info(std::span<const uint8_t> data, SpriteVariant variant)
      -> Result<nlohmann::json> {
    if (data.size() < kHeaderSize + kPartCount) {
      return {{}, Error::kInvalidFormat};
    }

    nlohmann::json out;
    out["direction"] = data[0];
    out["animation"] = data[1];

    if (variant == SpriteVariant::kMonster) {
      out["monster"] = read_u16_le(data.data() + 2);
    } else {
      const uint8_t sex_class = data[2];
      out["sex"] = static_cast<int>((sex_class & 0x80) != 0);
      out["class"] = static_cast<int>(sex_class & 0x0F);
      out["morph"] = data[3];
    }

    out["base"] = static_cast<int32_t>(read_u32_le(data.data() + 4));
    out["nspm"] = static_cast<int32_t>(read_u32_le(data.data() + 8));
    out["kit"] = static_cast<int32_t>(read_u32_le(data.data() + 12));

    nlohmann::json parts = nlohmann::json::object();
    size_t pos = kHeaderSize;
    for (size_t part = 0; part < kPartCount; ++part) {
      if (pos >= data.size()) {
        return {{}, Error::kInvalidFormat};
      }
      const size_t count = data[pos++];
      if (pos + count * kCellSize > data.size()) {
        return {{}, Error::kInvalidFormat};
      }
      nlohmann::json cells = nlohmann::json::array();
      for (size_t i = 0; i < count; ++i) {
        const uint16_t index = read_u16_le(data.data() + pos);
        const uint32_t id = read_u32_le(data.data() + pos + 2);
        pos += kCellSize;
        cells.push_back({{"index", index}, {"id", id}});
      }
      if (variant == SpriteVariant::kMonster) {
        parts[std::to_string(part)] = std::move(cells);
      } else {
        parts[kPlayerPartNames[part]] = std::move(cells);
      }
    }

    if (pos != data.size()) {
      return {{}, Error::kInvalidFormat};
    }
    out["parts"] = std::move(parts);
    return {std::move(out)};
  }

}  // namespace onex::archive
