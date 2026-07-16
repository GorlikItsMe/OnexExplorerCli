#pragma once

#include <onex/archive/archive_format.h>

namespace onex::archive {

  /// Parser for CCInf-format .NOS archives (e.g. NSgtdData, NSlangData).
  ///
  /// CCInf archives store entry metadata in a compact binary table indexed by
  /// a CCInf header block.  This parser handles both little-endian and native
  /// variants.
  class CcinfArchiveFormat final : public ArchiveFormat {
  public:
    auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> override;
  };

}  // namespace onex::archive
