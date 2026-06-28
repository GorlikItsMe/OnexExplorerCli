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

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.is_open_ = true;
    return Result<NosArchive>{result};
  }

}  // namespace onex::archive
