#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  /// One frame of an NSmpData / NSppData sprite entry.
  struct SpriteFrame {
    int width;                  ///< Frame width in pixels.
    int height;                 ///< Frame height in pixels.
    int x_origin;               ///< X origin offset from the frame anchor.
    int y_origin;               ///< Y origin offset from the frame anchor.
    std::vector<uint8_t> rgba;  ///< RGBA8 pixels, width*height*4 bytes.
  };

  /// Decodes the sprite frames of one NSmpData / NSppData entry.
  ///
  /// Layout (taletool sprites.md): `count(1)` frames, each a 12-byte
  /// descriptor {width(u16), height(u16), xOrigin(i16), yOrigin(i16),
  /// offset(u32)} followed (at `offset`) by GBAR4444 pixels
  /// (`width*2*height` bytes), converted to RGBA8.
  ///
  /// Descriptors with a zero width or height are unused frame slots (no
  /// pixels, no PNG representation) and are skipped.
  ///
  /// @return The decoded frames, or kInvalidFormat if the entry is truncated
  ///         or a frame's pixels extend past the end of the entry.
  auto decode_sprite_frames(std::span<const uint8_t> data) -> Result<std::vector<SpriteFrame>>;

  /// Encodes a frame's RGBA8 pixels to a PNG file via fpng.
  ///
  /// @return The PNG bytes, or kInvalidFormat if fpng encoding fails.
  auto encode_sprite_frame_to_png(const SpriteFrame& frame) -> Result<std::vector<uint8_t>>;

}  // namespace onex::archive
