#include <onex/downloader/build_info.h>

#include <nlohmann/json.hpp>
#include <vector>

namespace onex::downloader {

  using json = nlohmann::json;

  auto parse_build_info(const std::vector<byte>& raw) -> Result<BuildInfo> {
    try {
      auto j = json::parse(raw);
      BuildInfo info;
      info.total_size = j.value("totalSize", std::int64_t{0});
      info.build = j.value("build", 0);

      for (auto& e : j["entries"]) {
        BuildInfoEntry entry;
        entry.path = e.value("path", "");
        entry.sha1 = e.value("sha1", "");
        entry.file = e.value("file", "");
        entry.flags = e.value("flags", 0);
        entry.size = e.value("size", std::int64_t{0});
        entry.folder = e.value("folder", false);
        info.entries.push_back(std::move(entry));
      }
      return Result<BuildInfo>{std::move(info), Error::kNone};
    } catch (...) {
      return Result<BuildInfo>{{}, Error::kInvalidHeader};
    }
  }

}  // namespace onex::downloader
