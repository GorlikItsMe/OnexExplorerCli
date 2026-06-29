#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  // ---------------------------------------------------------------------------
  // Pixel format converters
  //
  // Each decode function takes raw NOS pixel bytes (no entry-specific header)
  // and returns 8-bit RGBA bytes in R,G,B,A order, row-major, width*height*4
  // bytes total. Each encode function is the inverse: RGBA8 in, NOS bytes out.
  //
  // The input span must contain exactly the bytes the format requires for the
  // given dimensions; otherwise an empty vector is returned.
  // ---------------------------------------------------------------------------

  // GBAR4444 -- ARGB, 4 bits per channel, little-endian. 2 bytes per pixel.
  auto decode_gbar4444_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_gbar4444(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // BGRA8888 -- 1 byte per channel, B,G,R,A order. 4 bytes per pixel.
  auto decode_bgra8888_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_bgra8888(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // ARGB555 -- 1-bit alpha, 5-bit RGB, packed as uint16 little-endian. 2 bytes
  // per pixel. Lossy: alpha collapses to 0/255 and channels expand by *8.
  auto decode_argb555_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_argb555(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // NSTC -- 1 byte palette index into the 16-entry NSTC palette. 1 byte per
  // pixel. Lossy: only palette colors are representable.
  auto decode_nstc_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_nstc(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // BGRA8888 interlaced -- same packing as BGRA8888 but pixels are laid out in
  // 256-pixel-wide column blocks. 4 bytes per pixel.
  auto decode_bgra8888_interlaced_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_bgra8888_interlaced(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // BARG4444 -- RGBA, 4 bits per channel, different byte order than GBAR.
  // 2 bytes per pixel.
  auto decode_barg4444_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_barg4444(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

  // Grayscale -- 1 luminance byte per pixel, expanded to (g,g,g,g). 1 byte per
  // pixel. Round-trips only when R=G=B=A.
  auto decode_grayscale_to_rgba(std::span<const uint8_t> pixels, int width, int height)
      -> std::vector<uint8_t>;
  auto encode_rgba_to_grayscale(std::span<const uint8_t> rgba, int width, int height)
      -> std::vector<uint8_t>;

}  // namespace onex::archive
