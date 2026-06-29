#include <onex/archive/nos_archive.h>

#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <vector>

#include "common.h"

namespace onex::cli {

  auto run_info(const std::string& filepath, const std::vector<int>& entry_ids, bool json_output)
      -> int {
    using onex::archive::NosArchive;

    auto result = NosArchive::open(filepath);
    if (!result) {
      std::cerr << "OnexExplorerCli: error: " << filepath << ": " << error_text(result.error)
                << "\n";
      return 1;
    }

    auto entries = result.value.entries();

    if (entry_ids.empty()) {
      if (json_output) {
        nlohmann::json j = nlohmann::json::array();
        for (const auto& entry : entries) {
          j.push_back(entry_to_json(entry));
        }
        std::cout << j.dump() << "\n";
      } else {
        std::printf("  ID  %-10s  %-12s  %-10s  %s\n", "Name", "Type", "Size", "Compressed");
        for (const auto& entry : entries) {
          std::printf("  %-3u  %-10s  %-12s  %-10" PRIu64 "  %s\n", entry.id, entry.name.c_str(),
                      entry_type_name(entry.type), entry.uncompressed_size,
                      entry.compressed ? "Yes" : "No");
        }
      }
      return 0;
    }

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
