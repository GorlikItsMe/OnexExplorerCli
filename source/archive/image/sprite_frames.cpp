#include <fpng.h>
#include <onex/archive/image/converter.h>
#include <onex/archive/image/sprite_frames.h>

#include <cstddef>
#include <cstdint>

namespace {

  struct FpngInit {
    FpngInit() { fpng::fpng_init(); }
  };
  const FpngInit init_fpng;

}  // namespace

namespace onex::archive {

  namespace {

    constexpr size_t kFrameDescriptorSize = 12;
    constexpr size_t kHeaderSize = 1;  // frame count byte

    auto read_u16_le(const uint8_t* p) -> int {
      return static_cast<int>(static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8);
    }

    auto read_i16_le(const uint8_t* p) -> int {
      return static_cast<int>(
          static_cast<int16_t>(static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1]) << 8));
    }

    auto read_u32_le(const uint8_t* p) -> uint32_t {
      return static_cast<uint32_t>(p[0]) | static_cast<uint32_t>(p[1]) << 8
             | static_cast<uint32_t>(p[2]) << 16 | static_cast<uint32_t>(p[3]) << 24;
    }

  }  // namespace

  auto decode_sprite_frames(std::span<const uint8_t> data) -> Result<std::vector<SpriteFrame>> {
    if (data.empty()) {
      return {{}, Error::kInvalidFormat};
    }

    const size_t frame_count = data[0];
    const size_t descriptors_size = kHeaderSize + frame_count * kFrameDescriptorSize;
    if (data.size() < descriptors_size) {
      return {{}, Error::kInvalidFormat};
    }

    std::vector<SpriteFrame> frames;
    frames.reserve(frame_count);
    for (size_t i = 0; i < frame_count; ++i) {
      const uint8_t* desc = data.data() + kHeaderSize + i * kFrameDescriptorSize;
      const int width = read_u16_le(desc);
      const int height = read_u16_le(desc + 2);
      const int x_origin = read_i16_le(desc + 4);
      const int y_origin = read_i16_le(desc + 6);
      const size_t pixel_offset = read_u32_le(desc + 8);

      // Zero-sized descriptors mark unused frame slots (seen as trailing
      // terminators and leading placeholders in NSmpData04); they store no
      // pixels and cannot be encoded as PNGs, so they are skipped.
      if (width == 0 || height == 0) {
        continue;
      }

      const size_t pixel_bytes = static_cast<size_t>(width) * 2 * static_cast<size_t>(height);
      if (pixel_offset > data.size() || pixel_bytes > data.size() - pixel_offset) {
        return {{}, Error::kInvalidFormat};
      }

      std::span<const uint8_t> pixels(data.data() + pixel_offset, pixel_bytes);
      auto rgba = decode_gbar4444_to_rgba(pixels, width, height);
      if (rgba.size() != static_cast<size_t>(width) * static_cast<size_t>(height) * 4) {
        return {{}, Error::kInvalidFormat};
      }

      frames.push_back(SpriteFrame{width, height, x_origin, y_origin, std::move(rgba)});
    }
    return {std::move(frames)};
  }

  auto encode_sprite_frame_to_png(const SpriteFrame& frame) -> Result<std::vector<uint8_t>> {
    std::vector<uint8_t> png;
    if (!fpng::fpng_encode_image_to_memory(frame.rgba.data(), static_cast<uint32_t>(frame.width),
                                           static_cast<uint32_t>(frame.height), 4, png,
                                           fpng::FPNG_ENCODE_SLOWER)) {
      return {{}, Error::kInvalidFormat};
    }
    return {std::move(png)};
  }

}  // namespace onex::archive
