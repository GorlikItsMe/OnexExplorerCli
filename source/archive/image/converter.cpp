#include <onex/archive/image/converter.h>

#include <array>
#include <cstddef>

namespace onex::archive {

  namespace {

    // 4-bit nibble (0..15) expanded to an 8-bit channel (0, 17, ..., 255).
    constexpr auto nibble_to_channel(uint8_t nibble) -> uint8_t {
      return static_cast<uint8_t>(nibble * 0x11);
    }

    // 8-bit channel (0..255) quantised to a 4-bit nibble (0..15).
    constexpr auto channel_to_nibble(uint8_t channel) -> uint8_t {
      return static_cast<uint8_t>(channel / 0x11);
    }

    // 5-bit field (0..31) expanded to an 8-bit channel (0, 8, ..., 248).
    constexpr auto five_to_channel(uint8_t bits) -> uint8_t {
      return static_cast<uint8_t>(bits * 8);
    }

    // 8-bit channel (0..255) quantised to a 5-bit field (0..31).
    constexpr auto channel_to_five(uint8_t channel) -> uint8_t {
      return static_cast<uint8_t>(channel / 8);
    }

    auto pixel_count(int width, int height) -> size_t {
      return static_cast<size_t>(width) * static_cast<size_t>(height);
    }

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

  // ---------------------------------------------------------------------------
  // GBAR4444
  // ---------------------------------------------------------------------------

  auto decode_gbar4444_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t> {
    const auto count = pixel_count(width, height);
    if (width <= 0 || height <= 0 || pixels.size() != count * 2) {
      return {};
    }
    std::vector<uint8_t> rgba(count * 4);
    for (size_t i = 0; i < count; ++i) {
      const uint8_t gb = pixels[i * 2];
      const uint8_t ar = pixels[i * 2 + 1];
      const uint8_t g = gb >> 4;
      const uint8_t b = gb & 0xF;
      const uint8_t a = ar >> 4;
      const uint8_t r = ar & 0xF;
      rgba[i * 4 + 0] = nibble_to_channel(r);
      rgba[i * 4 + 1] = nibble_to_channel(g);
      rgba[i * 4 + 2] = nibble_to_channel(b);
      rgba[i * 4 + 3] = nibble_to_channel(a);
    }
    return rgba;
  }

  auto encode_rgba_to_gbar4444(std::span<const uint8_t> rgba, int width, int height)
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
      pixels[i * 2 + 0] = static_cast<uint8_t>((channel_to_nibble(g) << 4) | channel_to_nibble(b));
      pixels[i * 2 + 1] = static_cast<uint8_t>((channel_to_nibble(a) << 4) | channel_to_nibble(r));
    }
    return pixels;
  }

  // ---------------------------------------------------------------------------
  // BGRA8888
  // ---------------------------------------------------------------------------

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

  // ---------------------------------------------------------------------------
  // ARGB555
  // ---------------------------------------------------------------------------

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

  // ---------------------------------------------------------------------------
  // NSTC
  // ---------------------------------------------------------------------------

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

  // ---------------------------------------------------------------------------
  // BGRA8888 interlaced
  // ---------------------------------------------------------------------------

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

  // ---------------------------------------------------------------------------
  // BARG4444
  // ---------------------------------------------------------------------------

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

  // ---------------------------------------------------------------------------
  // Grayscale
  // ---------------------------------------------------------------------------

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
