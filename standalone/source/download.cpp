#include <iostream>

#include "common.h"

namespace onex::cli {

  auto run_download(const std::string& output_dir, const std::string& build_id,
                    const std::vector<std::string>& archive_names, bool all) -> int {
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

    auto had_error = false;
    for (const auto& name : names) {
      auto resolved = downloader.resolve(manifest.value.entries, name);
      if (!resolved) {
        std::cerr << "OnexExplorerCli: error: " << name << ": " << error_text(resolved.error)
                  << "\n";
        had_error = true;
        continue;
      }

      auto status = downloader.download_file(resolved.value, output_dir);
      if (!status) {
        std::cerr << "OnexExplorerCli: error: " << resolved.value.file << ": "
                  << error_text(status.error) << "\n";
        had_error = true;
        continue;
      }

      if (status.value == GameforgeDownloader::FileStatus::kSkipped) {
        std::cout << "Skipped " << resolved.value.file << " (already up to date)\n";
      } else {
        std::cout << "Downloaded " << resolved.value.file << " (" << resolved.value.size
                  << " bytes)\n";
      }
    }

    return had_error ? 1 : 0;
  }

}  // namespace onex::cli
