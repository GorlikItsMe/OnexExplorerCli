#include <onex/archive/image/converter.h>

#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::channel_to_five;
  using detail::five_to_channel;
  using detail::pixel_count;

  auto decode_argb555_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count * 2) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint16_t value
          = static_cast<uint16_t>(pixels[i * 2]) | static_cast<uint16_t>(pixels[i * 2 + 1]) << 8;
      const uint8_t a = static_cast<uint8_t>(((value >> 15) & 0x1) * 255);
      const uint8_t r = five_to_channel((value >> 10) & 0x1F);
      const uint8_t g = five_to_channel((value >> 5) & 0x1F);
      const uint8_t b = five_to_channel(value & 0x1F);
      rgba[i * 4 + 0] = r;
      rgba[i * 4 + 1] = g;
      rgba[i * 4 + 2] = b;
      rgba[i * 4 + 3] = a;
    }
    return rgba;
  }

  auto encode_rgba_to_argb555(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count * 2);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t a_bit = static_cast<uint8_t>(rgba[i * 4 + 3] / 255);
      const uint16_t value = static_cast<uint16_t>(
          (a_bit << 15) | (channel_to_five(rgba[i * 4 + 0]) << 10)
          | (channel_to_five(rgba[i * 4 + 1]) << 5) | channel_to_five(rgba[i * 4 + 2]));
      pixels[i * 2 + 0] = static_cast<uint8_t>(value & 0xFF);
      pixels[i * 2 + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }
    return pixels;
  }

}  // namespace onex::archive
