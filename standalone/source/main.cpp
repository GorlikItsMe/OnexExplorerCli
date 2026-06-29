#include <onex/archive/entry.h>
#include <onex/archive/nos_archive.h>
#include <onex/downloader/downloader.h>
#include <onex/version.h>

#include <CLI/CLI.hpp>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {

  using onex::archive::NosArchive;
  using onex::downloader::GameforgeDownloader;

  auto error_text(onex::Error e) -> std::string {
    switch (e) {
      case onex::Error::kNetworkError:
        return "network error";
      case onex::Error::kEntryNotFound:
        return "archive not found";
      case onex::Error::kAmbiguousMatch:
        return "ambiguous archive name (multiple manifest entries match)";
      case onex::Error::kFileNotFound:
        return "file not found";
      case onex::Error::kReadError:
        return "read error";
      case onex::Error::kInvalidHeader:
        return "invalid manifest";
      case onex::Error::kInvalidFormat:
        return "invalid or unsupported archive format";
      case onex::Error::kIoError:
        return "I/O error";
      default:
        return "unknown error";
    }
  }

  auto run_download(const std::string& output_dir, const std::string& build_id,
                    const std::vector<std::string>& archive_names, bool all) -> int {
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

  auto is_text_type(onex::archive::EntryType type) -> bool {
    return type == onex::archive::EntryType::TextDat || type == onex::archive::EntryType::TextLst;
  }

  auto run_extract(const std::string& filepath, const std::string& output_dir,
                   const std::vector<int>& entry_ids) -> int {
    auto result = NosArchive::open(filepath);
    if (!result) {
      std::cerr << "OnexExplorerCli: error: " << filepath << ": " << error_text(result.error)
                << "\n";
      return 1;
    }

    auto& archive = result.value;
    auto entries = archive.entries();

    auto had_error = false;

    auto process_entry = [&](const onex::archive::EntryInfo& entry) {
      auto data = archive.read_entry(&entry - entries.data());
      if (!data) {
        std::cerr << "OnexExplorerCli: error: entry " << entry.id << " (" << entry.name
                  << "): " << error_text(data.error) << "\n";
        had_error = true;
        return;
      }

      auto out_name = entry.name;
      if (is_text_type(entry.type)) {
        for (auto* ext : {".dat", ".lst"}) {
          if (out_name.ends_with(ext)) {
            out_name.replace(out_name.size() - 4, 4, ".txt");
            break;
          }
        }
        if (out_name == entry.name) {
          out_name += ".txt";
        }
      } else {
        out_name += ".bin";
      }
      auto out_path = std::filesystem::path(output_dir) / out_name;
      std::filesystem::create_directories(out_path.parent_path());

      std::ofstream out{out_path, std::ios::binary};
      if (!out) {
        std::cerr << "OnexExplorerCli: error: cannot write " << out_path << "\n";
        had_error = true;
        return;
      }
      out.write(reinterpret_cast<const char*>(data.value.data()),
                static_cast<std::streamsize>(data.value.size()));
      std::cout << "Extracted " << out_name << " (" << data.value.size() << " bytes)\n";
    };

    if (entry_ids.empty()) {
      // Extract all entries
      for (const auto& entry : entries) {
        process_entry(entry);
      }
    } else {
      // Extract only matching entries by id
      for (auto id : entry_ids) {
        bool found = false;
        for (const auto& entry : entries) {
          if (static_cast<int>(entry.id) == id) {
            process_entry(entry);
            found = true;
            break;
          }
        }
        if (!found) {
          std::cerr << "OnexExplorerCli: error: entry " << id << " not found\n";
          had_error = true;
        }
      }
    }

    return had_error ? 1 : 0;
  }

  auto entry_type_name(onex::archive::EntryType type) -> const char* {
    switch (type) {
      case onex::archive::EntryType::Texture:
        return "Texture";
      case onex::archive::EntryType::Icon:
        return "Icon";
      case onex::archive::EntryType::Image4B:
        return "Image4B";
      case onex::archive::EntryType::TileGrid:
        return "TileGrid";
      case onex::archive::EntryType::TextDat:
        return "TextDat";
      case onex::archive::EntryType::TextLst:
        return "TextLst";
      case onex::archive::EntryType::Unknown:
        return "Unknown";
    }
    return "Unknown";
  }

  auto run_list(const std::string& filepath, bool json_output) -> int {
    auto result = NosArchive::open(filepath);
    if (!result) {
      std::cerr << "OnexExplorerCli: error: " << filepath << ": " << error_text(result.error)
                << "\n";
      return 1;
    }

    if (json_output) {
      nlohmann::json j = nlohmann::json::array();
      for (const auto& entry : result.value.entries()) {
        j.push_back({
            {"id", entry.id},
            {"name", entry.name},
            {"type", entry_type_name(entry.type)},
            {"creation_date", entry.creation_date},
            {"compressed", entry.compressed},
            {"offset", entry.offset},
            {"compressed_size", entry.compressed_size},
            {"uncompressed_size", entry.uncompressed_size},
        });
      }
      std::cout << j.dump(2) << "\n";
    } else {
      std::printf("  ID  %-10s  %-12s  %-10s  %s\n", "Name", "Type", "Size", "Compressed");
      for (const auto& entry : result.value.entries()) {
        std::printf("  %-3u  %-10s  %-12s  %-10" PRIu64 "  %s\n", entry.id, entry.name.c_str(),
                    entry_type_name(entry.type), entry.uncompressed_size,
                    entry.compressed ? "Yes" : "No");
      }
    }
    return 0;
  }

}  // namespace

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

  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    std::cout << "OnexExplorerCli, version " << ONEXEXPLORER_VERSION << std::endl;
    return 0;
  }

  if (download->parsed()) {
    return run_download(output_dir, build_id, archive_names, all);
  }

  if (extract->parsed()) {
    return run_extract(extract_path, extract_output_dir, extract_entry_ids);
  }

  if (list_cmd->parsed()) {
    return run_list(list_path, list_json);
  }

  std::cout << app.help() << std::endl;
  return 0;
}
