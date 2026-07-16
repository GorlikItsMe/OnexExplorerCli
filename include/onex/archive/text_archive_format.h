#pragma once

#include <onex/archive/archive_format.h>

namespace onex::archive {

  /// Parser for text-format .NOS archives (e.g. NScliData, NSetcData).
  ///
  /// These archives store their entry table as a textual listing rather than a
  /// binary index.  Each line describes one entry.
  class TextArchiveFormat final : public ArchiveFormat {
  public:
    auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> override;
  };

}  // namespace onex::archive
