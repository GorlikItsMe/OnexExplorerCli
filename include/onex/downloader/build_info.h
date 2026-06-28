#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <string>
#include <vector>

namespace onex::downloader {

  struct BuildInfoEntry {
    std::string path;
    std::string sha1;
    std::string file;
    int flags = 0;
    std::int64_t size = 0;
    bool folder = false;
  };

  struct BuildInfo {
    std::vector<BuildInfoEntry> entries;
    std::int64_t total_size = 0;
    int build = 0;
  };

  auto parse_build_info(const std::vector<byte>& raw) -> Result<BuildInfo>;

}  // namespace onex::downloader
