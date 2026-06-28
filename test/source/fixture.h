#pragma once

#include <onex/downloader/downloader.h>

#include <filesystem>
#include <string>

namespace {

  using onex::downloader::BuildInfoEntry;
  using onex::downloader::GameforgeDownloader;

  auto fixture_dir() -> std::filesystem::path {
    return std::filesystem::path(ONEX_PROJECT_SOURCE_DIR) / "temp" / "nostale";
  }

  auto output_path_for(const std::string& target_dir, const BuildInfoEntry& entry)
      -> std::filesystem::path {
    std::string normalized;
    normalized.reserve(entry.file.size());
    for (char c : entry.file) {
      normalized += (c == '\\') ? '/' : c;
    }
    return std::filesystem::path(target_dir) / normalized;
  }

  auto ensure_fixture(const std::string& name) -> std::filesystem::path {
    GameforgeDownloader d{"nostale", "latest"};
    auto manifest = d.fetch_manifest();
    if (!manifest) {
      throw std::runtime_error{"failed to fetch manifest"};
    }

    const BuildInfoEntry* pick = nullptr;
    for (const auto& entry : manifest.value.entries) {
      if (!entry.folder && entry.file == name) {
        pick = &entry;
        break;
      }
    }
    if (!pick) {
      throw std::runtime_error{"fixture not found in manifest: " + name};
    }

    auto dir = fixture_dir().string();
    auto status = d.download_file(*pick, dir);
    if (!status) {
      throw std::runtime_error{"failed to download fixture: " + name};
    }

    return output_path_for(dir, *pick);
  }

}  // namespace
