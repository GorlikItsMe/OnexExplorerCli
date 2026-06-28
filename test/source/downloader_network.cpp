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

TEST_CASE("download_file streams the smallest entry and size matches manifest") {
  GameforgeDownloader d{"nostale", "latest"};
  auto manifest = d.fetch_manifest();
  REQUIRE(manifest);

  // Check that the manifest is not empty
  CHECK_FALSE(manifest.value.entries.empty());

  // Find NostaleData\\NSetcData.NOS
  const BuildInfoEntry* pick = nullptr;
  for (const auto& entry : manifest.value.entries) {
    if (!entry.folder && entry.file == "NostaleData\\NSetcData.NOS") {
      pick = &entry;
    }
  }
  REQUIRE(pick != nullptr);

  // Create a temporary directory for the test
  auto dir = std::filesystem::temp_directory_path() / "onex_download_network_test";
  std::filesystem::remove_all(dir);

  // Download the file
  auto status = d.download_file(*pick, dir.string());
  REQUIRE(status);
  CHECK(status.value == GameforgeDownloader::FileStatus::kDownloaded);

  // Check that the file exists and has the correct size
  auto out = output_path_for(dir.string(), *pick);
  REQUIRE(std::filesystem::exists(out));
  CHECK(std::filesystem::file_size(out) == static_cast<std::uintmax_t>(pick->size));

  // Check that the file has the correct SHA1
  if (!pick->sha1.empty()) {
    CHECK(onex::downloader::sha1_file_hex(out.string()) == pick->sha1);
  }

  // Download the file again (should be skipped)
  auto second = d.download_file(*pick, dir.string());
  REQUIRE(second);
  CHECK(second.value == GameforgeDownloader::FileStatus::kSkipped);

  // Remove the temporary directory
  std::filesystem::remove_all(dir);
}
