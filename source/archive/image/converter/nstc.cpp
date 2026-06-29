#include <onex/archive/image/converter.h>

#include <array>
#include <cstdint>
#include <vector>

#include "common.h"

namespace onex::archive {

  using detail::pixel_count;

  namespace {

    // NSTC palette: index -> (r, g, b). Alpha is always 255 on decode.
    struct NstcEntry {
      uint8_t index;
      uint8_t r;
      uint8_t g;
      uint8_t b;
    };

    constexpr std::array<NstcEntry, 13> kNstcPalette{{
        {0x00, 255, 255, 255},
        {0x01, 100, 100, 100},
        {0x02, 100, 150, 100},
        {0x03, 0, 0, 0},
        {0x09, 100, 100, 150},
        {0x0A, 0, 50, 200},
        {0x0B, 150, 100, 100},
        {0x0D, 150, 150, 100},
        {0x10, 150, 100, 150},
        {0x11, 0, 200, 50},
        {0x12, 200, 50, 0},
        {0x13, 250, 230, 10},
        {0xFF, 150, 0, 255},
    }};

    // Fallback colour for palette indices that are not explicitly defined.
    constexpr NstcEntry kNstcFallback = {0xFF, 150, 0, 255};

    auto nstc_lookup(uint8_t index) -> NstcEntry {
      for (const auto& entry : kNstcPalette) {
        if (entry.index == index) {
          return entry;
        }
      }
      return kNstcFallback;
    }

    auto nstc_reverse(uint8_t r, uint8_t g, uint8_t b) -> uint8_t {
      for (const auto& entry : kNstcPalette) {
        if (entry.r == r && entry.g == g && entry.b == b) {
          return entry.index;
        }
      }
      return 0xFF;
    }

  }  // namespace

  auto decode_nstc_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const auto color = nstc_lookup(pixels[i]);
      rgba[i * 4 + 0] = color.r;
      rgba[i * 4 + 1] = color.g;
      rgba[i * 4 + 2] = color.b;
      rgba[i * 4 + 3] = 255;
    }
    return rgba;
  }

  auto encode_rgba_to_nstc(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || rgba.size() != count * 4) {
      return {};
    }
    std::vector<uint8_t> pixels(count);
    for (size_t i = 0; i < count; ++i) {
      pixels[i] = nstc_reverse(rgba[i * 4 + 0], rgba[i * 4 + 1], rgba[i * 4 + 2]);
    }
    return pixels;
  }

}  // namespace onex::archive
