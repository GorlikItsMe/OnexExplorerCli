#pragma once

#include <onex/core/error.h>
#include <onex/downloader/build_info.h>
#include <onex/downloader/http_client.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace onex::downloader {

  /// Result status of a single file download.
  enum class FileStatus {
    kDownloaded,  ///< File was successfully downloaded.
    kSkipped,     ///< File already exists on disk with a matching SHA1.
  };

  /// Result of one file in a batch download.
  struct BatchResult {
    std::size_t index;          ///< Index of the entry in the input vector.
    Result<FileStatus> status;  ///< Download status (success/error).
  };

  /// Downloads .NOS archives from the Gameforge CDN.
  ///
  /// Handles manifest fetching, name resolution, SHA1 verification, and
  /// parallel batch downloads via a thread pool.
  class GameforgeDownloader {
  public:
    /// Backward compatibility alias for the moved FileStatus enum.
    using FileStatus = onex::downloader::FileStatus;

    /// Construct a downloader for the given game and build.
    /// @param game_id   Game identifier (e.g. "Nostale").
    /// @param build_id  Build version string (e.g. "latest" or a numeric ID).
    /// @param http      Optional custom HttpClient (defaults to CurlHttpClient).
    explicit GameforgeDownloader(std::string game_id, std::string build_id,
                                 std::unique_ptr<HttpClient> http
                                 = std::make_unique<CurlHttpClient>());

    ~GameforgeDownloader();

    /// Fetch and parse the CDN build manifest.
    auto fetch_manifest() -> Result<BuildInfo>;

    /// Resolve an archive name against a manifest into a single entry.
    /// Tries exact file-field match first, then bare filename fallback.
    /// Returns kAmbiguousMatch if more than one entry matches.
    auto resolve(const std::vector<BuildInfoEntry>& entries, const std::string& name)
        -> Result<BuildInfoEntry>;

    /// Download a single file and verify its SHA1.
    /// Skips if a file with a matching SHA1 already exists.
    auto download_file(const BuildInfoEntry& entry, const std::string& target_dir)
        -> Result<FileStatus>;

    /// Set a progress callback for sequential downloads.
    /// Called during curl transfers with (dltotal, dlnow, ultotal, ulnow).
    void set_progress_callback(ProgressCallback cb);

    /// Set a factory for creating HttpClient instances (e.g. in worker threads).
    /// Default produces CurlHttpClient. Swap to FakeHttpClient in tests.
    void set_client_factory(HttpClientFactory factory);

    /// Download multiple files in parallel using up to max_concurrent threads.
    /// Returns one result per entry in the same order as the input vector.
    auto download_batch(const std::vector<BuildInfoEntry>& entries, const std::string& target_dir,
                        int max_concurrent) -> std::vector<BatchResult>;

  private:
    auto make_manifest_url() const -> std::string;
    auto make_download_url(const BuildInfoEntry& entry) const -> std::string;
    auto verify_sha1(const std::string& path, const std::string& expected_hex) -> bool;
    auto download_file_with_client(const BuildInfoEntry& entry, const std::string& target_dir,
                                   HttpClient& http) -> Result<FileStatus>;

    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace onex::downloader
