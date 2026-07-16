#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  /// Decode a decompressed image entry into a PNG byte buffer.
  ///
  /// Parses the entry-specific header (texture 8-byte, tile-grid/icon/image4b
  /// 4-/13-byte), selects the pixel format from EntryType (and the texture
  /// format byte), decodes the raw pixels to RGBA8, and encodes the RGBA8 to
  /// PNG via lodepng.
  ///
  /// @param decompressed_data  The full decompressed entry body.
  /// @param type               The entry type (must be an image type).
  /// @return A complete PNG file on success, or kInvalidFormat if the entry is
  ///         not an image, the header is truncated, pixel dimensions mismatch,
  ///         or lodepng encoding fails.
  auto decode_entry_to_png(std::span<const uint8_t> decompressed_data, EntryType type)
      -> Result<std::vector<uint8_t>>;

}  // namespace onex::archive
