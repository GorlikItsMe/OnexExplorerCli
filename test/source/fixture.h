#pragma once

#include <onex/downloader/downloader.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

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
    constexpr int kMaxAttempts = 3;

    GameforgeDownloader d{"nostale", "latest"};

    BuildInfoEntry pick_entry{};
    bool found = false;

    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
      auto manifest = d.fetch_manifest();
      if (!manifest) {
        if (attempt < kMaxAttempts) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
          continue;
        }
        throw std::runtime_error{"failed to fetch manifest"};
      }

      for (const auto& entry : manifest.value.entries) {
        if (!entry.folder && entry.file == name) {
          pick_entry = entry;
          found = true;
          break;
        }
      }

      if (!found) {
        throw std::runtime_error{"fixture not found in manifest: " + name};
      }

      auto dir = fixture_dir().string();
      auto status = d.download_file(pick_entry, dir);
      if (status) {
        return output_path_for(dir, pick_entry);
      }

      if (attempt < kMaxAttempts) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

    throw std::runtime_error{"failed to download fixture: " + name};
  }

}  // namespace
