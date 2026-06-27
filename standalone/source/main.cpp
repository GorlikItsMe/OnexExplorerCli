#include <curl/curl.h>
#include <onex/version.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <onex/download.hpp>

auto main(int argc, char** argv) -> int {
  CLI::App app{"OnexExplorerCli - a tool for unpacking and repacking .NOS data files"};

  bool show_version = false;
  app.add_flag("-v,--version", show_version, "Print the current version number");

  std::vector<std::string> archive_names;
  std::string output_dir;
  std::string build_id = "latest";

  auto* download_cmd = app.add_subcommand("download", "Download .NOS archives from Gameforge");
  download_cmd->add_option("archive-names", archive_names, "Archive name(s) to download")
      ->required();
  download_cmd->add_option("-o,--output", output_dir, "Target directory for downloaded files")
      ->required();
  download_cmd->add_option("--build-id", build_id, "Build version (default: latest)");

  app.require_subcommand(0, 1);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  if (show_version) {
    std::cout << "OnexExplorerCli, version " << ONEXEXPLORER_VERSION << std::endl;
    return 0;
  }

  if (download_cmd->parsed()) {
    curl_global_init(CURL_GLOBAL_ALL);
    int result = onex::run_download({.archive_names = std::move(archive_names),
                                     .output_dir = std::move(output_dir),
                                     .build_id = std::move(build_id)});
    curl_global_cleanup();
    return result;
  }

  return 0;
}
