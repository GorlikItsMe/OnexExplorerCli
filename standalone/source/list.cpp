#include <onex/archive/nos_archive.h>

#include <cinttypes>
#include <cstdio>
#include <iostream>

#include "common.h"

namespace onex::cli {

  auto run_list(const std::string& filepath, bool json_output) -> int {
    using onex::archive::NosArchive;

    auto result = NosArchive::open(filepath);
    if (!result) {
      std::cerr << "OnexExplorerCli: error: " << filepath << ": " << error_text(result.error)
                << "\n";
      return 1;
    }

    if (json_output) {
      nlohmann::json j = nlohmann::json::array();
      for (const auto& entry : result.value.entries()) {
        j.push_back(entry_to_json(entry));
      }
      std::cout << j.dump() << "\n";
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

}  // namespace onex::cli
