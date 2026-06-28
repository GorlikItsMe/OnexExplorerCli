#include <onex/archive/archive_format.h>
#include <onex/archive/zlib_archive_format.h>

namespace onex::archive {

  auto ArchiveFormat::detect(std::span<const uint8_t> header) -> std::unique_ptr<ArchiveFormat> {
    if (header.size() < 7) {
      return nullptr;
    }
    if (std::equal(header.begin(), header.begin() + 7, "NT Data")) {
      return std::make_unique<ZlibArchiveFormat>();
    }
    if (header.size() >= 10) {
      if (std::equal(header.begin(), header.begin() + 10, "32GBS V1.0")
          || std::equal(header.begin(), header.begin() + 10, "ITEMS V1.0")) {
        return std::make_unique<ZlibArchiveFormat>();
      }
    }
    return nullptr;
  }

}  // namespace onex::archive
