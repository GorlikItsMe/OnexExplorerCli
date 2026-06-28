#pragma once

#include <onex/core/error.h>

#include <array>
#include <string>
#include <vector>

namespace onex::downloader {

  auto sha1_hex(const std::vector<byte>& data) -> std::string;
  auto sha1_file_hex(const std::string& path) -> std::string;

}  // namespace onex::downloader
