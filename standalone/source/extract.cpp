#include <onex/archive/image/entry_image.h>
#include <onex/archive/nos_archive.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "common.h"

namespace onex::cli {

  auto run_extract(const std::string& filepath, const std::string& output_dir,
                   const std::vector<int>& entry_ids) -> int {
    using onex::archive::NosArchive;

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

      const bool is_image = entry.type == onex::archive::EntryType::Texture
                            || entry.type == onex::archive::EntryType::Icon
                            || entry.type == onex::archive::EntryType::Image4B
                            || entry.type == onex::archive::EntryType::TileGrid;

      auto out_name = entry.name;
      const uint8_t* write_data = data.value.data();
      auto write_size = data.value.size();
      std::vector<uint8_t> png_bytes;

      if (is_image) {
        auto png = onex::archive::decode_entry_to_png(data.value, entry.type);
        if (png) {
          png_bytes = std::move(png.value);
          write_data = png_bytes.data();
          write_size = png_bytes.size();
          out_name += ".png";
        } else {
          out_name += ".bin";
        }
      }

      auto out_path = std::filesystem::path(output_dir) / out_name;
      std::error_code ec;
      if (!std::filesystem::is_directory(out_path.parent_path(), ec)) {
        if (!std::filesystem::create_directories(out_path.parent_path(), ec) && ec) {
          std::cerr << "OnexExplorerCli: error: cannot create output directory \""
                    << out_path.parent_path() << "\"\\n";
          had_error = true;
          return;
        }
      }

      std::ofstream out{out_path, std::ios::binary};
      if (!out) {
        std::cerr << "OnexExplorerCli: error: cannot write " << out_path << "\n";
        had_error = true;
        return;
      }
      out.write(reinterpret_cast<const char*>(write_data),
                static_cast<std::streamsize>(write_size));
      std::cout << "Extracted " << out_name << " (" << write_size << " bytes)\n";
    };

    if (entry_ids.empty()) {
      for (const auto& entry : entries) {
        process_entry(entry);
      }
    } else {
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

}  // namespace onex::cli
