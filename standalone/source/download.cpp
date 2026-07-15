#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "common.h"

namespace onex::cli {

  namespace {

    auto short_name(const std::string& file) -> std::string {
      auto pos = file.find_last_of('\\');
      if (pos != std::string::npos) {
        return file.substr(pos + 1);
      }
      return file;
    }

    void draw_progress_bar(const std::string& name, int pct, double speed_mb,
                           std::chrono::seconds eta) {
      // Progress bar: 20 chars
      int bar_width = 20;
      int filled = pct * bar_width / 100;
      if (filled > bar_width) filled = bar_width;

      std::string bar(filled, '=');
      if (filled < bar_width) bar += '>';
      bar.append(bar_width - bar.size(), ' ');

      std::cout << '\r' << name << "  [" << bar << "]  " << pct << "%  " << std::fixed
                << std::setprecision(1) << speed_mb << " MB/s";

      if (eta.count() > 0) {
        auto mins = std::chrono::duration_cast<std::chrono::minutes>(eta);
        auto secs = eta - mins;
        if (mins.count() > 0) {
          std::cout << "  ETA " << mins.count() << "m" << secs.count() << "s";
        } else {
          std::cout << "  ETA " << secs.count() << "s";
        }
      }

      std::cout << "    " << std::flush;  // trailing spaces to erase previous
    }

  }  // namespace

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
        auto& entry = resolved[result.index];
        if (result.status) {
          if (result.status.value == FileStatus::kDownloaded) {
            std::cout << "Downloaded " << entry.file << " (" << entry.size << " bytes)\n";
          } else {
            std::cout << "Skipped " << entry.file << " (already up to date)\n";
          }
        } else {
          std::cerr << "OnexExplorerCli: error: " << entry.file << ": "
                    << error_text(result.status.error) << "\n";
          had_error = 1;
        }
      }
      return had_error;
    }

    // Sequential download with live progress bar
    auto had_error = false;
    auto is_tty = isatty(fileno(stdout));
    for (const auto& entry : resolved) {
      auto short_name_str = short_name(entry.file);

      if (is_tty) {
        // Set up a one-shot progress callback for this file
        using onex::downloader::ProgressCallback;
        using clock = std::chrono::steady_clock;

        auto last_dl = std::make_shared<std::int64_t>(0);
        auto last_tp = std::make_shared<clock::time_point>(clock::now());

        ProgressCallback cb = [&entry, short_name_str, last_dl, last_tp](
                                  std::int64_t dltotal, std::int64_t dlnow, std::int64_t,
                                  std::int64_t) {
          if (dltotal <= 0) return;

          auto now = clock::now();
          auto elapsed = std::chrono::duration<double>(now - *last_tp).count();
          auto pct = static_cast<int>(dlnow * 100 / dltotal);

          // Compute instantaneous speed over the last ~0.5s
          double speed_mb = 0.0;
          auto dl_delta = dlnow - *last_dl;
          if (elapsed > 0.3 && dl_delta > 0) {
            speed_mb = dl_delta / elapsed / (1024.0 * 1024.0);
            *last_dl = dlnow;
            *last_tp = now;
          }

          if (dltotal > 0 && dlnow > 0) {
            auto remaining_s = static_cast<int>((dltotal - dlnow) / (dlnow / elapsed));
            draw_progress_bar(short_name_str, pct, speed_mb, std::chrono::seconds{remaining_s});
          }
        };

        downloader.set_progress_callback(std::move(cb));
      }

      auto status = downloader.download_file(entry, output_dir);

      // Clear the progress bar line (only if we drew one)
      if (is_tty) {
        std::cout << '\r' << std::string(60, ' ') << '\r' << std::flush;
      }

      if (!status) {
        std::cerr << "OnexExplorerCli: error: " << entry.file << ": " << error_text(status.error)
                  << "\n";
        had_error = true;
        continue;
      }

      if (status.value == onex::downloader::FileStatus::kSkipped) {
        std::cout << "Skipped " << entry.file << " (already up to date)\n";
      } else {
        std::cout << "Downloaded " << entry.file << " (" << entry.size << " bytes)\n";
      }
    }

    // Clear the progress callback so the shared client isn't affected
    downloader.set_progress_callback({});

    return had_error ? 1 : 0;
  }

}  // namespace onex::cli
