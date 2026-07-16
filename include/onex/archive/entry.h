#pragma once

#include <cstdint>
#include <string>

namespace onex::archive {

  struct Codec;

  /// Known entry types in a .NOS archive.
  enum class EntryType {
    Texture,   ///< NStpData, NStpeData, NStpuData (sub-types 7, 11, 12)
    Icon,      ///< NSipData (24), ITEMS V1.0 (103)
    Image4B,   ///< NS4BbData (101, 32GBS V1.0)
    TileGrid,  ///< NStcData (5)
    TextDat,   ///< .dat file within a TextArchive
    TextLst,   ///< .lst file within a TextArchive
    Unknown,
  };

  /// Metadata for a single entry in a .NOS archive.
  struct EntryInfo {
    uint32_t id;                   ///< Numeric entry identifier.
    std::string name;              ///< Entry name (e.g. filename).
    uint32_t creation_date;        ///< Entry creation timestamp (DOS date format).
    bool compressed;               ///< Whether the entry body is compressed.
    EntryType type;                ///< Classified entry type.
    uint64_t offset;               ///< Byte offset of the entry body in the archive.
    uint64_t compressed_size;      ///< Size of the compressed entry body.
    uint64_t uncompressed_size;    ///< Size after decompression.
    const Codec* codec = nullptr;  ///< Codec used for compression/decompression.
  };

}  // namespace onex::archive
