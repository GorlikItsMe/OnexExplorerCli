#pragma once

#include <onex/archive/archive_format.h>

namespace onex::archive {

  /// Parser for zlib-compressed .NOS archives (e.g. NS4BbData, NSipData).
  ///
  /// The entry table itself is stored after a compressed sub-header whose
  /// sub-type determines the kind of entries (images, icons, textures, …).
  class ZlibArchiveFormat final : public ArchiveFormat {
  public:
    auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> override;

  private:
    /// Detect the numeric sub-type from the archive header.
    static auto detect_sub_type(std::span<const uint8_t> header) -> int;
    /// Map a numeric sub-type to the corresponding EntryType enum value.
    static auto sub_type_to_entry_type(int sub_type) -> EntryType;
  };

}  // namespace onex::archive
