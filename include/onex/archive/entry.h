#pragma once

#include <cstdint>
#include <string>

namespace onex::archive {

  struct Codec;

  enum class EntryType {
    Texture,   // NStpData, NStpeData, NStpuData (sub-types 7, 11, 12)
    Icon,      // NSipData (24), ITEMS V1.0 (103)
    Image4B,   // NS4BbData (101, 32GBS V1.0)
    TileGrid,  // NStcData (5)
    TextDat,   // .dat file within a TextArchive
    TextLst,   // .lst file within a TextArchive
    Unknown,
  };

  struct EntryInfo {
    uint32_t id;
    std::string name;
    uint32_t creation_date;
    bool compressed;
    EntryType type;
    uint64_t offset;
    uint64_t compressed_size;
    uint64_t uncompressed_size;
    const Codec* codec = nullptr;
  };

}  // namespace onex::archive
