#include <onex/archive/image/converter.h>

#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::channel_to_nibble;
  using detail::nibble_to_channel;
  using detail::pixel_count;

  auto decode_barg4444_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count * 2) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t ba = pixels[i * 2];
      const uint8_t rg = pixels[i * 2 + 1];
      const uint8_t b = ba >> 4;
      const uint8_t a = ba & 0xF;
      const uint8_t r = rg >> 4;
      const uint8_t g = rg & 0xF;
      rgba[i * 4 + 0] = nibble_to_channel(r);
      rgba[i * 4 + 1] = nibble_to_channel(g);
      rgba[i * 4 + 2] = nibble_to_channel(b);
      rgba[i * 4 + 3] = nibble_to_channel(a);
    }
    return rgba;
  }

  auto encode_rgba_to_barg4444(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count * 2);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t r = rgba[i * 4 + 0];
      const uint8_t g = rgba[i * 4 + 1];
      const uint8_t b = rgba[i * 4 + 2];
      const uint8_t a = rgba[i * 4 + 3];
      pixels[i * 2 + 0] = static_cast<uint8_t>((channel_to_nibble(b) << 4) | channel_to_nibble(a));
      pixels[i * 2 + 1] = static_cast<uint8_t>((channel_to_nibble(r) << 4) | channel_to_nibble(g));
    }
    return pixels;
  }

}  // namespace onex::archive
