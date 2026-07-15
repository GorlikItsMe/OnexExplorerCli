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

    bool header_starts_with(const onex::archive::NosArchive::Header& header, const char* prefix,
                            size_t len) {
      for (size_t i = 0; i < len; ++i) {
        if (static_cast<char>(header[i]) != prefix[i]) return false;
      }
      return true;
    }

    auto detect_format_name(const onex::archive::NosArchive::Header& header) -> std::string {
      constexpr size_t kHeaderSize = 0x10;
      if (header_starts_with(header, "NT Data", 7)) {
        std::string name = "NT Data";
        for (size_t i = 7; i < kHeaderSize; ++i) {
          char c = static_cast<char>(header[i]);
          if (c == '\0' || !std::isprint(static_cast<unsigned char>(c))) break;
          name += c;
        }
        return name;
      }
      if (header_starts_with(header, "32GBS V1.0", 10)) return "32GBS V1.0";
      if (header_starts_with(header, "ITEMS V1.0", 10)) return "ITEMS V1.0";
      if (header_starts_with(header, "CCINF V1.20", 11)) return "CCINF V1.20";
      // Try to read as readable ASCII string
      std::string s;
      bool ascii_only = true;
      for (size_t i = 0; i < kHeaderSize; ++i) {
        char c = static_cast<char>(header[i]);
        if (c == '\0') break;
        if (!std::isprint(static_cast<unsigned char>(c))) {
          ascii_only = false;
          break;
        }
        s += c;
      }
      // Only trust it if it looks like a format name (starts with letter, min 3 chars)
      if (ascii_only && s.size() >= 3 && std::isalpha(static_cast<unsigned char>(s[0]))) {
        return s;
      }
      return "Unknown (TextArchive)";
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
      auto format = detect_format_name(archive.header());
      auto entry_count = archive.entries().size();

      if (json_output) {
        nlohmann::json j;
        j["file"] = filepath;
        j["file_size"] = file_size;
        j["format"] = format;
        j["entry_count"] = entry_count;
        std::cout << j.dump() << "\n";
      } else {
        std::printf("File:             %s\n", filepath.c_str());
        std::printf("File size:        %" PRIu64 "\n", file_size);
        std::printf("Format:           %s\n", format.c_str());
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
