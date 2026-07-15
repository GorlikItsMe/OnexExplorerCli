#include <onex/archive/nos_archive.h>

#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "common.h"

namespace onex::cli {

  namespace {

    auto header_hex(const onex::archive::NosArchive::Header& header) -> std::string {
      std::string out;
      char buf[4];
      for (size_t i = 0; i < header.size(); ++i) {
        if (i > 0) out += ' ';
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned char>(header[i]));
        out += buf;
      }
      return out;
    }

    auto header_text(const onex::archive::NosArchive::Header& header) -> std::string {
      std::string out;
      for (auto b : header) {
        auto c = static_cast<unsigned char>(b);
        out += std::isprint(c) && c != 0x00 ? static_cast<char>(c) : '.';
      }
      return out;
    }

  }  // namespace

  auto run_info(const std::string& filepath, const std::vector<int>& entry_ids, bool json_output)
      -> int {
    using onex::archive::NosArchive;

    auto result = NosArchive::open(filepath);
    if (!result) {
      std::cerr << "OnexExplorerCli: error: " << filepath << ": " << error_text(result.error)
                << "\n";
      return 1;
    }

    const auto& archive = result.value;

    // No entry IDs → show archive-level info
    if (entry_ids.empty()) {
      auto file_size = std::filesystem::file_size(filepath);
      auto h_hex = header_hex(archive.header());
      auto h_text = header_text(archive.header());
      auto entry_count = archive.entries().size();

      if (json_output) {
        nlohmann::json j;
        j["file"] = filepath;
        j["file_size"] = file_size;
        j["header_hex"] = h_hex;
        j["header_text"] = h_text;
        j["entry_count"] = entry_count;
        std::cout << j.dump() << "\n";
      } else {
        std::printf("File:             %s\n", filepath.c_str());
        std::printf("File size:        %" PRIu64 " bytes\n", file_size);
        std::printf("Header hex:      %s\n", h_hex.c_str());
        std::printf("Header text:     %s\n", h_text.c_str());
        std::printf("Entry count:      %zu\n", entry_count);
      }
      return 0;
    }

    // Entry IDs provided → show entry-level details
    auto entries = archive.entries();
    std::vector<const onex::archive::EntryInfo*> matched;
    for (auto id : entry_ids) {
      const onex::archive::EntryInfo* found = nullptr;
      for (const auto& entry : entries) {
        if (static_cast<int>(entry.id) == id) {
          found = &entry;
          break;
        }
      }
      if (!found) {
        std::cerr << "OnexExplorerCli: error: entry " << id << " not found\n";
        return 1;
      }
      matched.push_back(found);
    }

    if (matched.size() == 1) {
      const auto& entry = *matched[0];
      if (json_output) {
        std::cout << entry_to_json(entry).dump() << "\n";
      } else {
        std::printf("ID:               %u\n", entry.id);
        std::printf("Name:             %s\n", entry.name.c_str());
        std::printf("Type:             %s\n", entry_type_name(entry.type));
        std::printf("Creation Date:    %u\n", entry.creation_date);
        std::printf("Compressed:       %s\n", entry.compressed ? "Yes" : "No");
        std::printf("Offset:           %" PRIu64 "\n", entry.offset);
        std::printf("Compressed Size:  %" PRIu64 "\n", entry.compressed_size);
        std::printf("Uncompressed Size: %" PRIu64 "\n", entry.uncompressed_size);
      }
    } else {
      if (json_output) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto* entry : matched) {
          j.push_back(entry_to_json(*entry));
        }
        std::cout << j.dump() << "\n";
      } else {
        std::printf("  ID  %-10s  %-12s  %-10s  %s\n", "Name", "Type", "Size", "Compressed");
        for (const auto* entry : matched) {
          std::printf("  %-3u  %-10s  %-12s  %-10" PRIu64 "  %s\n", entry->id, entry->name.c_str(),
                      entry_type_name(entry->type), entry->uncompressed_size,
                      entry->compressed ? "Yes" : "No");
        }
      }
    }
    return 0;
  }

}  // namespace onex::cli
