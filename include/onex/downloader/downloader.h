#pragma once

#include <onex/core/error.h>
#include <onex/downloader/build_info.h>
#include <onex/downloader/http_client.h>

#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  class GameforgeDownloader {
  public:
    enum class FileStatus {
      kDownloaded,
      kSkipped,  // already on disk with matching SHA1
    };

    explicit GameforgeDownloader(std::string game_id, std::string build_id,
                                 std::unique_ptr<HttpClient> http
                                 = std::make_unique<CurlHttpClient>());

    ~GameforgeDownloader();

    auto fetch_manifest() -> Result<BuildInfo>;

    auto resolve(const std::vector<BuildInfoEntry>& entries, const std::string& name)
        -> Result<BuildInfoEntry>;

    auto download_file(const BuildInfoEntry& entry, const std::string& target_dir)
        -> Result<FileStatus>;

    /// Download multiple files in parallel using a thread pool.
    /// Returns one result per entry in the same order as the input vector.
    /// Folders and empty entries are skipped with FileStatus::kSkipped.
    auto download_files(const std::vector<BuildInfoEntry>& entries,
                        const std::string& target_dir,
                        int max_concurrency = 4) -> std::vector<Result<FileStatus>>;

  private:
    auto make_manifest_url() const -> std::string;
    auto verify_sha1(const std::string& path, const std::string& expected_hex) -> bool;

    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace onex::downloader
