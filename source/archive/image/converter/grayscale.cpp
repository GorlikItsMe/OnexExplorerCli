#include <onex/archive/image/converter.h>

#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::pixel_count;

  auto decode_grayscale_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t gray = pixels[i];
      rgba[i * 4 + 0] = gray;
      rgba[i * 4 + 1] = gray;
      rgba[i * 4 + 2] = gray;
      rgba[i * 4 + 3] = gray;
    }
    return rgba;
  }

  auto encode_rgba_to_grayscale(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count);
    for (size_t i = 0; i < count; ++i) {
      pixels[i] = rgba[i * 4 + 1];
    }
    return pixels;
  }

}  // namespace onex::archive
