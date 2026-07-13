#include <iostream>

#include "common.h"

namespace onex::cli {

  auto run_download(const std::string& output_dir, const std::string& build_id,
                    const std::vector<std::string>& archive_names, bool all, int jobs) -> int {
    using onex::downloader::GameforgeDownloader;

    GameforgeDownloader downloader{"nostale", build_id};

    auto manifest = downloader.fetch_manifest();
    if (!manifest) {
      std::cerr << "OnexExplorerCli: error: failed to fetch manifest: "
                << error_text(manifest.error) << "\n";
      return 1;
    }

    auto names = archive_names;
    if (all) {
      names.clear();
      for (const auto& entry : manifest.value.entries) {
        if (!entry.folder) {
          names.push_back(entry.file);
        }
      }
    }

    // Resolve all entries upfront
    std::vector<onex::downloader::BuildInfoEntry> resolved;
    resolved.reserve(names.size());

    for (const auto& name : names) {
      auto r = downloader.resolve(manifest.value.entries, name);
      if (!r) {
        std::cerr << "OnexExplorerCli: error: " << name << ": " << error_text(r.error) << "\n";
        continue;
      }
      resolved.push_back(r.value);
    }

    if (resolved.empty()) {
      return 1;
    }

    if (jobs > 1) {
      using namespace onex::downloader;

      // Parallel download via batch
      auto results = downloader.download_batch(resolved, output_dir, jobs);

      int had_error = 0;
      for (auto& result : results) {
        if (result.status) {
          if (result.status.value == FileStatus::kDownloaded) {
            std::cout << "Downloaded " << result.entry.file << " (" << result.entry.size
                      << " bytes)\n";
          } else {
            std::cout << "Skipped " << result.entry.file << " (already up to date)\n";
          }
        } else {
          std::cerr << "OnexExplorerCli: error: " << result.entry.file << ": "
                    << error_text(result.status.error) << "\n";
          had_error = 1;
        }
      }
      return had_error;
    }

    // Sequential download (original behaviour)
    auto had_error = false;
    for (const auto& entry : resolved) {
      auto status = downloader.download_file(entry, output_dir);
      if (!status) {
        std::cerr << "OnexExplorerCli: error: " << entry.file << ": "
                  << error_text(status.error) << "\n";
        had_error = true;
        continue;
      }

      if (status.value == onex::downloader::FileStatus::kSkipped) {
        std::cout << "Skipped " << entry.file << " (already up to date)\n";
      } else {
        std::cout << "Downloaded " << entry.file << " (" << entry.size << " bytes)\n";
      }
    }

    return had_error ? 1 : 0;
  }

}  // namespace onex::cli
