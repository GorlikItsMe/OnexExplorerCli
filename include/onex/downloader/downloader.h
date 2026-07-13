#pragma once

#include <onex/core/error.h>
#include <onex/downloader/build_info.h>
#include <onex/downloader/http_client.h>

#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  enum class FileStatus {
    kDownloaded,
    kSkipped,  // already on disk with matching SHA1
  };

  struct BatchResult {
    BuildInfoEntry entry;
    Result<FileStatus> status;
  };

  class GameforgeDownloader {
  public:
    explicit GameforgeDownloader(std::string game_id, std::string build_id,
                                 std::unique_ptr<HttpClient> http
                                 = std::make_unique<CurlHttpClient>());

    ~GameforgeDownloader();

    auto fetch_manifest() -> Result<BuildInfo>;

    auto resolve(const std::vector<BuildInfoEntry>& entries, const std::string& name)
        -> Result<BuildInfoEntry>;

    auto download_file(const BuildInfoEntry& entry, const std::string& target_dir)
        -> Result<FileStatus>;

    /// Download multiple files in parallel using up to max_concurrent threads.
    /// Returns one result per entry in the same order as the input vector.
    auto download_batch(const std::vector<BuildInfoEntry>& entries,
                        const std::string& target_dir, int max_concurrent)
        -> std::vector<BatchResult>;

  private:
    auto make_manifest_url() const -> std::string;
    auto make_download_url(const BuildInfoEntry& entry) const -> std::string;
    auto verify_sha1(const std::string& path, const std::string& expected_hex) -> bool;
    auto download_file_with_client(const BuildInfoEntry& entry,
                                   const std::string& target_dir, HttpClient& http)
        -> Result<FileStatus>;

    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace onex::downloader
