#pragma once

#include <onex/archive/archive_format.h>

namespace onex::archive {

  class ZlibArchiveFormat final : public ArchiveFormat {
  public:
    auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> override;

  private:
    static auto detect_sub_type(std::span<const uint8_t> header) -> int;
    static auto sub_type_to_entry_type(int sub_type) -> EntryType;
  };

}  // namespace onex::archive
