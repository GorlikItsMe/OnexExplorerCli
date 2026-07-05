#pragma once

#include <onex/archive/archive_format.h>

namespace onex::archive {

  class CcinfArchiveFormat final : public ArchiveFormat {
  public:
    auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> override;
  };

}  // namespace onex::archive
