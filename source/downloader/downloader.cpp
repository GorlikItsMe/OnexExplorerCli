#include <onex/downloader/downloader.h>
#include <onex/downloader/sha1.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace onex::downloader {

  namespace fs = std::filesystem;

  namespace {

    auto manifest_relative_path(const std::string& file) -> fs::path {
      std::string normalized;
      normalized.reserve(file.size());
      for (char c : file) {
        normalized += (c == '\\') ? '/' : c;
      }
      return fs::path{normalized};
    }

    auto bare_filename(const std::string& file) -> std::string {
      return manifest_relative_path(file).filename().string();
    }

    auto find_matches(const std::vector<BuildInfoEntry>& entries, const std::string& name,
                      bool (*key)(const BuildInfoEntry&, const std::string&))
        -> std::vector<const BuildInfoEntry*> {
      std::vector<const BuildInfoEntry*> out;
      for (const auto& entry : entries) {
        if (entry.folder || entry.file.empty()) {
          continue;
        }
        if (key(entry, name)) {
          out.push_back(&entry);
        }
      }
      return out;
    }

    auto key_exact(const BuildInfoEntry& entry, const std::string& name) -> bool {
      return entry.file == name;
    }

    auto key_bare(const BuildInfoEntry& entry, const std::string& name) -> bool {
      return bare_filename(entry.file) == name;
    }

  }  // namespace

  struct GameforgeDownloader::Impl {
    std::string game_id;
    std::string build_id;
    std::unique_ptr<HttpClient> http;
  };

  GameforgeDownloader::GameforgeDownloader(std::string game_id, std::string build_id,
                                           std::unique_ptr<HttpClient> http)
      : impl_(std::make_unique<Impl>()) {
    impl_->game_id = std::move(game_id);
    impl_->build_id = std::move(build_id);
    impl_->http = std::move(http);
  }

  GameforgeDownloader::~GameforgeDownloader() = default;

  auto GameforgeDownloader::fetch_manifest() -> Result<BuildInfo> {
    auto url = make_manifest_url();
    auto resp = impl_->http->get(url);
    if (resp.status_code != 200) {
      return Result<BuildInfo>{{}, Error::kNetworkError};
    }
    return parse_build_info(resp.body);
  }

  auto GameforgeDownloader::resolve(const std::vector<BuildInfoEntry>& entries,
                                    const std::string& name) -> Result<BuildInfoEntry> {
    auto exact = find_matches(entries, name, key_exact);
    if (exact.size() == 1) {
      return Result<BuildInfoEntry>{*exact.front(), Error::kNone};
    }
    if (exact.size() > 1) {
      return Result<BuildInfoEntry>{{}, Error::kAmbiguousMatch};
    }

    auto bare = find_matches(entries, name, key_bare);
    if (bare.size() == 1) {
      return Result<BuildInfoEntry>{*bare.front(), Error::kNone};
    }
    if (bare.size() > 1) {
      return Result<BuildInfoEntry>{{}, Error::kAmbiguousMatch};
    }

    return Result<BuildInfoEntry>{{}, Error::kEntryNotFound};
  }

  auto GameforgeDownloader::download_file(const BuildInfoEntry& entry,
                                          const std::string& target_dir) -> Result<FileStatus> {
    if (entry.folder || entry.file.empty()) {
      return Result<FileStatus>{FileStatus::kSkipped, Error::kNone};
    }

    auto output_path = fs::path(target_dir) / manifest_relative_path(entry.file);

    if (fs::exists(output_path) && verify_sha1(output_path.string(), entry.sha1)) {
      return Result<FileStatus>{FileStatus::kSkipped, Error::kNone};
    }

    std::error_code ec;
    fs::create_directories(output_path.parent_path(), ec);
    if (ec) {
      return Result<FileStatus>{FileStatus::kSkipped, Error::kIoError};
    }

    auto url = std::string{"http://patches.gameforge.com"} + entry.path;
    auto resp = impl_->http->download(url, output_path.string());
    if (resp.status_code != 200) {
      return Result<FileStatus>{FileStatus::kSkipped, Error::kNetworkError};
    }

    return Result<FileStatus>{FileStatus::kDownloaded, Error::kNone};
  }

  auto GameforgeDownloader::download_files(const std::vector<BuildInfoEntry>& entries,
                                            const std::string& target_dir,
                                            int max_concurrency) -> std::vector<Result<FileStatus>> {
    // Collect entries that actually need downloading
    struct WorkItem {
      const BuildInfoEntry* entry;
      std::size_t index;
    };
    std::vector<WorkItem> work;
    work.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
      const auto& e = entries[i];
      if (!e.folder && !e.file.empty()) {
        work.push_back({&e, i});
      }
    }

    // Pre-fill results with skipped for folders/empty entries
    std::vector<Result<FileStatus>> results(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
      const auto& e = entries[i];
      if (e.folder || e.file.empty()) {
        results[i] = Result<FileStatus>{FileStatus::kSkipped, Error::kNone};
      }
    }

    if (work.empty()) {
      return results;
    }

    std::atomic<std::size_t> next_index{0};

    auto worker = [&]() {
      auto http = impl_->http->clone();
      while (true) {
        auto idx = next_index.fetch_add(1, std::memory_order_relaxed);
        if (idx >= work.size()) {
          break;
        }
        const auto& item = work[idx];
        const auto& entry = *item.entry;

        auto output_path = fs::path(target_dir) / manifest_relative_path(entry.file);

        if (fs::exists(output_path) && verify_sha1(output_path.string(), entry.sha1)) {
          results[item.index] = Result<FileStatus>{FileStatus::kSkipped, Error::kNone};
          continue;
        }

        std::error_code ec;
        fs::create_directories(output_path.parent_path(), ec);
        if (ec) {
          results[item.index] = Result<FileStatus>{FileStatus::kSkipped, Error::kIoError};
          continue;
        }

        auto url = std::string{"http://patches.gameforge.com"} + entry.path;
        auto resp = http->download(url, output_path.string());
        if (resp.status_code != 200) {
          results[item.index] = Result<FileStatus>{FileStatus::kSkipped, Error::kNetworkError};
          continue;
        }

        results[item.index] = Result<FileStatus>{FileStatus::kDownloaded, Error::kNone};
      }
    };

    auto n_threads = std::min(static_cast<std::size_t>(max_concurrency), work.size());
    if (n_threads < 1) {
      n_threads = 1;
    }

    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (std::size_t i = 0; i < n_threads; ++i) {
      threads.emplace_back(worker);
    }
    for (auto& t : threads) {
      t.join();
    }

    return results;
  }

  auto GameforgeDownloader::make_manifest_url() const -> std::string {
    if (impl_->build_id == "latest") {
      return "https://spark.gameforge.com/api/v1/patching/download/latest/" + impl_->game_id
             + "/default";
    }
    return "https://spark.gameforge.com/api/v1/patching/download/install/" + impl_->game_id
           + "/default/" + impl_->build_id;
  }

  auto GameforgeDownloader::verify_sha1(const std::string& path, const std::string& expected_hex)
      -> bool {
    if (expected_hex.empty()) {
      return false;
    }
    auto actual = sha1_file_hex(path);
    return actual == expected_hex;
  }

}  // namespace onex::downloader
