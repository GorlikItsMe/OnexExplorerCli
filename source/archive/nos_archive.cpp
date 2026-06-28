#include <onex/archive/archive_format.h>
#include <onex/archive/nos_archive.h>

#include <fstream>

namespace onex::archive {

  auto NosArchive::open(const std::filesystem::path& filepath) -> Result<NosArchive> {
    if (!std::filesystem::exists(filepath)) {
      return {NosArchive{}, Error::kFileNotFound};
    }

    std::ifstream file{filepath, std::ios::binary};
    if (!file.is_open()) {
      return {NosArchive{}, Error::kReadError};
    }

    Header header{};
    if (!file.read(reinterpret_cast<char*>(header.data()), kNosHeaderSize)) {
      return {NosArchive{}, Error::kReadError};
    }

    auto format = ArchiveFormat::detect(header);
    if (!format) {
      return {NosArchive{}, Error::kInvalidFormat};
    }

    auto entries = format->parse_entry_table(header, file);
    if (!entries) {
      return {NosArchive{}, entries.error};
    }

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.is_open_ = true;
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
  }

}  // namespace onex::archive
