#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <string>
#include <vector>

namespace onex::downloader {

  /// A single entry in the Gameforge CDN build manifest.
  struct BuildInfoEntry {
    std::string path;       ///< Relative CDN path to the file.
    std::string sha1;       ///< Hex-encoded SHA1 hash of the file.
    std::string file;       ///< Filename (may include sub-directory prefix).
    int flags = 0;          ///< Entry flags from the manifest.
    std::int64_t size = 0;  ///< File size in bytes.
    bool folder = false;    ///< Whether this entry is a directory.
  };

  /// Parsed Gameforge CDN build manifest.
  struct BuildInfo {
    std::vector<BuildInfoEntry> entries;  ///< All files in the manifest.
    std::int64_t total_size = 0;          ///< Sum of all entry sizes.
    int build = 0;                        ///< Build version number.
  };

  /// Parse a raw manifest response into a BuildInfo structure.
  /// Returns kInvalidFormat if the manifest cannot be decoded.
  auto parse_build_info(const std::vector<byte>& raw) -> Result<BuildInfo>;

}  // namespace onex::downloader
