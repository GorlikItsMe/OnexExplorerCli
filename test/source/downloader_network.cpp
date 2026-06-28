#include <doctest/doctest.h>
#include <onex/downloader/downloader.h>
#include <onex/downloader/sha1.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

  using onex::downloader::BuildInfoEntry;
  using onex::downloader::GameforgeDownloader;

  auto output_path_for(const std::string& target_dir, const BuildInfoEntry& entry)
      -> std::filesystem::path {
    std::string normalized;
    normalized.reserve(entry.file.size());
    for (char c : entry.file) {
      normalized += (c == '\\') ? '/' : c;
    }
    return std::filesystem::path(target_dir) / normalized;
  }

}  // namespace

TEST_CASE("fetch_manifest returns a non-empty manifest from Gameforge CDN") {
  GameforgeDownloader d{"nostale", "latest"};
  auto manifest = d.fetch_manifest();
  REQUIRE(manifest);
  CHECK_FALSE(manifest.value.entries.empty());
  CHECK(manifest.value.build > 0);
}

TEST_CASE("download_file streams the smallest entry and size matches manifest") {
  GameforgeDownloader d{"nostale", "latest"};
  auto manifest = d.fetch_manifest();
  REQUIRE(manifest);

  const BuildInfoEntry* pick = nullptr;
  for (const auto& entry : manifest.value.entries) {
    if (entry.folder || entry.file.empty() || entry.size <= 0) {
      continue;
    }
    if (pick == nullptr || entry.size < pick->size) {
      pick = &entry;
    }
  }
  REQUIRE(pick != nullptr);

  auto dir = std::filesystem::temp_directory_path() / "onex_download_network_test";
  std::filesystem::remove_all(dir);

  auto status = d.download_file(*pick, dir.string());
  REQUIRE(status);
  CHECK(status.value == GameforgeDownloader::FileStatus::kDownloaded);

  auto out = output_path_for(dir.string(), *pick);
  REQUIRE(std::filesystem::exists(out));
  CHECK(std::filesystem::file_size(out) == static_cast<std::uintmax_t>(pick->size));

  if (!pick->sha1.empty()) {
    CHECK(onex::downloader::sha1_file_hex(out.string()) == pick->sha1);
  }

  std::filesystem::remove_all(dir);
}

TEST_CASE("download_file skips an entry already on disk with matching SHA1") {
  GameforgeDownloader d{"nostale", "latest"};
  auto manifest = d.fetch_manifest();
  REQUIRE(manifest);

  const BuildInfoEntry* pick = nullptr;
  for (const auto& entry : manifest.value.entries) {
    if (entry.folder || entry.file.empty() || entry.size <= 0 || entry.sha1.empty()) {
      continue;
    }
    if (pick == nullptr || entry.size < pick->size) {
      pick = &entry;
    }
  }
  REQUIRE(pick != nullptr);

  auto dir = std::filesystem::temp_directory_path() / "onex_download_skip_test";
  std::filesystem::remove_all(dir);

  REQUIRE(d.download_file(*pick, dir.string()));

  auto second = d.download_file(*pick, dir.string());
  REQUIRE(second);
  CHECK(second.value == GameforgeDownloader::FileStatus::kSkipped);

  std::filesystem::remove_all(dir);
}
