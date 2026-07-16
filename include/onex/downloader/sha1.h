#pragma once

#include <onex/core/error.h>

#include <array>
#include <string>
#include <vector>

namespace onex::downloader {

  /// Compute the hex-encoded SHA1 hash of a byte buffer.
  auto sha1_hex(const std::vector<byte>& data) -> std::string;

  /// Compute the hex-encoded SHA1 hash of a file on disk.
  /// Returns an empty string if the file cannot be read.
  auto sha1_file_hex(const std::string& path) -> std::string;

}  // namespace onex::downloader
