#pragma once

namespace onex::archive {

  // The seven NOS pixel formats used by the original OnexExplorer. Each format
  // describes how raw pixel bytes are packed before/after conversion to RGBA8.
  enum class PixelFormat {
    kGbar4444,            // ARGB 4 bits per channel, little-endian byte order
    kBgra8888,            // 1 byte per channel, B,G,R,A order
    kArgb555,             // 1-bit alpha, 5-bit RGB, packed as uint16 little-endian
    kNstc,                // 1 byte palette index into the 16-entry NSTC palette
    kBgra8888Interlaced,  // BGRA8888 with 256-pixel column interlaced scan order
    kBarg4444,            // RGBA 4 bits per channel, different byte order than GBAR
    kGrayscale,           // 1 luminance byte per pixel
  };

}  // namespace onex::archive
