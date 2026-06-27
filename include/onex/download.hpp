#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace onex {

  struct ManifestEntry {
    std::string file;
    std::string path;
    int64_t size{};
    std::string sha1;
  };

  struct DownloadOptions {
    std::vector<std::string> archive_names;
    std::string output_dir;
    std::string build_id = "latest";
  };

  std::string basename(std::string_view path);

  std::vector<ManifestEntry> fetch_manifest(std::string_view build_id);

  const ManifestEntry* match_archive(const std::vector<ManifestEntry>& manifest,
                                     std::string_view name);

  int run_download(const DownloadOptions& opts);

}  // namespace onex
