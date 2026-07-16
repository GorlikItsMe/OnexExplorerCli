#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <istream>
#include <memory>
#include <span>
#include <vector>

namespace onex::archive {

  /// Abstract base for .NOS archive format parsers.
  ///
  /// Each .NOS file starts with a 16-byte header whose first four bytes identify
  /// the archive sub-format.  Derived classes implement the entry-table layout
  /// for their specific format (CCInf, text, zlib-compressed, …).
  struct ArchiveFormat {
    virtual ~ArchiveFormat() = default;

    /// Parse the entry table from an open archive stream.
    /// @param header  The 16-byte archive header.
    /// @param stream  The archive stream (positioned after the header).
    /// @return A vector of EntryInfo, or an error on parse failure.
    virtual auto parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
        -> Result<std::vector<EntryInfo>> = 0;

    /// Detect the archive format from its header and return a matching parser.
    /// Returns nullptr if the header is not recognised.
    static auto detect(std::span<const uint8_t> header) -> std::unique_ptr<ArchiveFormat>;
  };

}  // namespace onex::archive
