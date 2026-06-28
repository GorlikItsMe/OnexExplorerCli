#include <onex/downloader/downloader.h>
#include <onex/version.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace {

  using onex::downloader::GameforgeDownloader;

  auto error_text(onex::Error e) -> std::string {
    switch (e) {
      case onex::Error::kNetworkError:
        return "network error";
      case onex::Error::kEntryNotFound:
        return "archive not found";
      case onex::Error::kAmbiguousMatch:
        return "ambiguous archive name (multiple manifest entries match)";
      case onex::Error::kInvalidHeader:
        return "invalid manifest";
      case onex::Error::kIoError:
        return "I/O error";
      default:
        return "unknown error";
    }
  }

  auto run_download(const std::string& output_dir, const std::string& build_id,
                    const std::vector<std::string>& archive_names) -> int {
    GameforgeDownloader downloader{"nostale", build_id};

    auto manifest = downloader.fetch_manifest();
    if (!manifest) {
      std::cerr << "OnexExplorerCli: error: failed to fetch manifest: "
                << error_text(manifest.error) << "\n";
      return 1;
    }

    auto had_error = false;
    for (const auto& name : archive_names) {
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

}  // namespace

auto main(int argc, char** argv) -> int {
  CLI::App app{"OnexExplorerCli - a tool for unpacking and repacking .NOS data files"};

  bool show_version = false;
  app.add_flag("-v,--version", show_version, "Print the current version number");

  // download subcommand
  std::string output_dir;
  std::string build_id = "latest";
  std::vector<std::string> archive_names;

  auto* download = app.add_subcommand("download", "Fetch .NOS archives from the Gameforge CDN");
  download->add_option("-o,--output", output_dir, "Target directory for downloaded files")
      ->required();
  download->add_option("--build-id", build_id, "Build version (default: latest)")
      ->capture_default_str();
  download->add_option("archive-names", archive_names, "Archive names from the Gameforge manifest")
      ->required()
      ->expected(-1);

  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    std::cout << "OnexExplorerCli, version " << ONEXEXPLORER_VERSION << std::endl;
    return 0;
  }

  if (download->parsed()) {
    return run_download(output_dir, build_id, archive_names);
  }

  std::cout << app.help() << std::endl;
  return 0;
}
