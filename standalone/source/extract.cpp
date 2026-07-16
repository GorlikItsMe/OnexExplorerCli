#include <onex/archive/image/entry_image.h>
#include <onex/archive/nos_archive.h>
#include <onex/util/thread_pool.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
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

    // --- Phase 1: determine which entries to extract ---
    std::vector<size_t> indices;
    if (entry_ids.empty()) {
      indices.reserve(entries.size());
      for (size_t i = 0; i < entries.size(); ++i) indices.push_back(i);
    } else {
      for (auto id : entry_ids) {
        bool found = false;
        for (size_t i = 0; i < entries.size(); ++i) {
          if (static_cast<int>(entries[i].id) == id) {
            indices.push_back(i);
            found = true;
            break;
          }
        }
        if (!found) {
          std::cerr << "OnexExplorerCli: error: entry " << id << " not found\n";
        }
      }
      if (indices.empty()) return 1;
    }

    // --- Phase 2: read + decompress all target entries (fast, sequential I/O) ---
    struct DecodedEntry {
      const onex::archive::EntryInfo* info;
      std::vector<uint8_t> data;
    };

    std::vector<DecodedEntry> decoded;
    decoded.reserve(indices.size());
    bool had_error = false;

    for (auto idx : indices) {
      auto& entry = entries[idx];
      auto data = archive.read_entry(idx);
      if (!data) {
        std::cerr << "OnexExplorerCli: error: entry " << entry.id << " (" << entry.name
                  << "): " << error_text(data.error) << "\n";
        had_error = true;
        continue;
      }
      decoded.push_back({&entry, std::move(data.value)});
    }

    if (decoded.empty()) return had_error ? 1 : 0;

    // --- Phase 3: PNG encode + write in parallel ---
    onex::util::ThreadPool pool;
    std::atomic<bool> worker_error{false};
    std::mutex cout_mutex;

    [[maybe_unused]] auto _errors = pool.parallel_for(decoded.size(), [&](size_t idx) -> bool {
      const auto& de = decoded[idx];
      const auto& entry = *de.info;

      const bool is_image = entry.type == onex::archive::EntryType::Texture
                            || entry.type == onex::archive::EntryType::Icon
                            || entry.type == onex::archive::EntryType::Image4B
                            || entry.type == onex::archive::EntryType::TileGrid;

      auto out_name = entry.name;
      const uint8_t* write_data = de.data.data();
      auto write_size = de.data.size();
      std::vector<uint8_t> png_bytes;

      if (is_image) {
        auto png = onex::archive::decode_entry_to_png(de.data, entry.type);
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
          std::lock_guard lk(cout_mutex);
          std::cerr << "OnexExplorerCli: error: cannot create output directory \""
                    << out_path.parent_path() << "\"\n";
          worker_error = true;
          return false;
        }
      }

      std::ofstream out{out_path, std::ios::binary};
      if (!out) {
        std::lock_guard lk(cout_mutex);
        std::cerr << "OnexExplorerCli: error: cannot write " << out_path << "\n";
        worker_error = true;
        return false;
      }
      out.write(reinterpret_cast<const char*>(write_data),
                static_cast<std::streamsize>(write_size));

      {
        std::lock_guard lk(cout_mutex);
        std::cout << "Extracted " << out_name << " (" << write_size << " bytes)\n";
      }

      return true;
    });

    return (had_error || worker_error) ? 1 : 0;
  }

}  // namespace onex::cli
