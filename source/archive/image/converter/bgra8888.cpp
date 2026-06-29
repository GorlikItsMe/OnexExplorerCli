#include <onex/archive/image/converter.h>

#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::pixel_count;

  auto decode_bgra8888_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t b = pixels[i * 4 + 0];
      const uint8_t g = pixels[i * 4 + 1];
      const uint8_t r = pixels[i * 4 + 2];
      const uint8_t a = pixels[i * 4 + 3];
      rgba[i * 4 + 0] = r;
      rgba[i * 4 + 1] = g;
      rgba[i * 4 + 2] = b;
      rgba[i * 4 + 3] = a;
    }
    return rgba;
  }

  auto encode_rgba_to_bgra8888(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t r = rgba[i * 4 + 0];
      const uint8_t g = rgba[i * 4 + 1];
      const uint8_t b = rgba[i * 4 + 2];
      const uint8_t a = rgba[i * 4 + 3];
      pixels[i * 4 + 0] = b;
      pixels[i * 4 + 1] = g;
      pixels[i * 4 + 2] = r;
      pixels[i * 4 + 3] = a;
    }
    return pixels;
  }

}  // namespace onex::archive
