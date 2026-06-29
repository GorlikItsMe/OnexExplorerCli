#include <onex/archive/image/converter.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::pixel_count;

  namespace {

    // Advances the interlaced cursor to the next (x, y) position. Pixels are
    // laid out in 256-pixel-wide column blocks: within a block starting at
    // column ``num`` the cursor sweeps x in [num, min(num + 256, width)) and y
    // in [0, height), then moves to the next block at num + 256.
    auto advance_interlaced(int& x, int& y, int& num, int width, int height) -> void {
      ++x;
      if (x == num + 256 || x == width) {
        x = num;
        ++y;
      }
      if (y == height) {
        y = 0;
        num += 256;
        x = num;
      }
    }

  }  // namespace

  auto decode_bgra8888_interlaced_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    int x = 0, y = 0, num = 0;
    for (size_t i = 0; i < count; ++i) {
      const auto dst = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
      const uint8_t b = pixels[i * 4 + 0];
      const uint8_t g = pixels[i * 4 + 1];
      const uint8_t r = pixels[i * 4 + 2];
      const uint8_t a = pixels[i * 4 + 3];
      rgba[dst * 4 + 0] = r;
      rgba[dst * 4 + 1] = g;
      rgba[dst * 4 + 2] = b;
      rgba[dst * 4 + 3] = a;
      advance_interlaced(x, y, num, width, height);
    }
    return rgba;
  }

  auto encode_rgba_to_bgra8888_interlaced(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count * 4);
    int x = 0, y = 0, num = 0;
    for (size_t i = 0; i < count; ++i) {
      const auto src = static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
      const uint8_t r = rgba[src * 4 + 0];
      const uint8_t g = rgba[src * 4 + 1];
      const uint8_t b = rgba[src * 4 + 2];
      const uint8_t a = rgba[src * 4 + 3];
      pixels[i * 4 + 0] = b;
      pixels[i * 4 + 1] = g;
      pixels[i * 4 + 2] = r;
      pixels[i * 4 + 3] = a;
      advance_interlaced(x, y, num, width, height);
    }
    return pixels;
  }

}  // namespace onex::archive
