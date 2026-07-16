#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <istream>
#include <memory>
#include <span>
#include <vector>

namespace onex::archive {

  struct ArchiveFormat {
    virtual ~ArchiveFormat() = default;

    virtual auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> = 0;

    static auto detect(std::span<const uint8_t> header) -> std::unique_ptr<ArchiveFormat>;
  };

}  // namespace onex::archive
