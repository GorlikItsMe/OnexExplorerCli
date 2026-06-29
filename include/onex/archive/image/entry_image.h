#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  // Decode a decompressed image entry into a PNG byte buffer. Parses the
  // entry-specific header (texture 8-byte, tile-grid/icon/image4b 4-/13-byte),
  // selects the pixel format from EntryType (and the texture format byte),
  // decodes the raw pixels to RGBA8, and encodes the RGBA8 to PNG via lodepng.
  //
  // Returns kInvalidFormat when the entry type is not an image, the header is
  // truncated, the pixel data does not match the declared dimensions, or lodepng
  // fails to encode. On success the returned bytes are a complete PNG file.
  auto decode_entry_to_png(std::span<const uint8_t> decompressed_data, EntryType type)
      -> Result<std::vector<uint8_t>>;

}  // namespace onex::archive
