#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>
#include <onex/downloader/downloader.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace onex::cli {

  auto error_text(onex::Error e) -> std::string;

  auto entry_type_name(onex::archive::EntryType type) -> const char*;

  auto entry_to_json(const onex::archive::EntryInfo& entry) -> nlohmann::json;

  auto run_download(const std::string& output_dir, const std::string& build_id,
                    const std::vector<std::string>& archive_names, bool all) -> int;

  auto run_extract(const std::string& filepath, const std::string& output_dir,
                   const std::vector<int>& entry_ids) -> int;

  auto run_list(const std::string& filepath, bool json_output) -> int;

  auto run_info(const std::string& filepath, const std::vector<int>& entry_ids, bool json_output)
      -> int;

}  // namespace onex::cli
