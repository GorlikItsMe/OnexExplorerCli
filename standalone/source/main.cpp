#include <onex/version.h>

#include <CLI/CLI.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "common.h"

auto main(int argc, char** argv) -> int {
  CLI::App app{"OnexExplorerCli - a tool for unpacking and repacking .NOS data files"};

  bool show_version = false;
  app.add_flag("-v,--version", show_version, "Print the current version number");

  // download subcommand
  std::string output_dir;
  std::string build_id = "latest";
  bool all = false;
  std::vector<std::string> archive_names;

  auto* download = app.add_subcommand("download", "Fetch .NOS archives from the Gameforge CDN");
  download->add_option("-o,--output", output_dir, "Target directory for downloaded files")
      ->required();
  download->add_option("--build-id", build_id, "Build version (default: latest)")
      ->capture_default_str();
  download->add_flag("--all", all, "Download all archives from the manifest");
  download->add_option("archive-names", archive_names, "Archive names from the Gameforge manifest")
      ->expected(-1);

  // extract subcommand
  std::string extract_path;
  std::string extract_output_dir;
  std::vector<int> extract_entry_ids;
  auto* extract = app.add_subcommand("extract", "Extract entries from a .NOS archive");
  extract->add_option("file", extract_path, "Path to a .NOS archive file")->required();
  extract->add_option("-o,--output", extract_output_dir, "Output directory")->required();
  extract->add_option("--entry", extract_entry_ids, "Entry ID(s) to extract (repeatable)")
      ->expected(-1);

  // list subcommand
  std::string list_path;
  bool list_json = false;
  auto* list_cmd = app.add_subcommand("list", "List entries in a .NOS archive");
  list_cmd->add_option("file", list_path, "Path to a .NOS archive file")->required();
  list_cmd->add_flag("--json", list_json, "Output as JSON");

  // info subcommand
  std::string info_path;
  std::vector<int> info_entry_ids;
  bool info_json = false;
  auto* info_cmd = app.add_subcommand("info", "Show details for entries in a .NOS archive");
  info_cmd->add_option("file", info_path, "Path to a .NOS archive file")->required();
  info_cmd->add_option("--entry", info_entry_ids, "Entry ID(s) to show (repeatable, default: all)")
      ->expected(-1);
  info_cmd->add_flag("--json", info_json, "Output as JSON");

  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    std::cout << "OnexExplorerCli, version " << ONEXEXPLORER_VERSION << std::endl;
    return 0;
  }

  if (download->parsed()) {
    return onex::cli::run_download(output_dir, build_id, archive_names, all);
  }

  if (extract->parsed()) {
    return onex::cli::run_extract(extract_path, extract_output_dir, extract_entry_ids);
  }

  if (list_cmd->parsed()) {
    return onex::cli::run_list(list_path, list_json);
  }

  if (info_cmd->parsed()) {
    return onex::cli::run_info(info_path, info_entry_ids, info_json);
  }

  std::cout << app.help() << std::endl;
  return 0;
}
