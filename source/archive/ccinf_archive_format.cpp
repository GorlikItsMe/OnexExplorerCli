#include <onex/archive/ccinf_archive_format.h>
#include <onex/archive/nos_archive.h>

#include <array>
#include <optional>

namespace onex::archive {

  namespace {

    auto read_u32_le(std::istream& s) -> std::optional<uint32_t> {
      std::array<char, 4> buf{};
      if (!s.read(buf.data(), 4)) {
        return std::nullopt;
      }
      return static_cast<uint32_t>(static_cast<unsigned char>(buf[0]))
             | static_cast<uint32_t>(static_cast<unsigned char>(buf[1])) << 8
             | static_cast<uint32_t>(static_cast<unsigned char>(buf[2])) << 16
             | static_cast<uint32_t>(static_cast<unsigned char>(buf[3])) << 24;
    }

    auto read_u16_le(std::istream& s) -> std::optional<uint16_t> {
      std::array<char, 2> buf{};
      if (!s.read(buf.data(), 2)) {
        return std::nullopt;
      }
      return static_cast<uint16_t>(static_cast<unsigned char>(buf[0]))
             | static_cast<uint16_t>(static_cast<unsigned char>(buf[1])) << 8;
    }

  }  // namespace

  auto CcinfArchiveFormat::parse_entry_table(std::span<const uint8_t> /*header*/,
                                             std::istream& stream)
      -> Result<std::vector<EntryInfo>> {
    // CCINF V1.20 format layout:
    //   0x00 - 0x0B: 12-byte header magic "CCINF V1.20\x1a"
    //   0x0C - 0x0F: creationDate (uint32 LE)
    //   0x10 - 0x13: fileSize (uint32 LE) - total content size
    //   0x14 - 0x17: fileSize again (uint32 LE) - same value repeated
    //   0x18:        separator (1 byte, always 0x00)
    //   0x19 - 0x1C: fileAmount (uint32 LE) - number of entries
    //   Then entry records follow sequentially

    stream.seekg(0);
    if (!stream) {
      return {{}, Error::kReadError};
    }

    // Skip 12-byte header magic
    stream.seekg(12, std::ios::cur);
    if (!stream) {
      return {{}, Error::kInvalidFormat};
    }

    auto creation_date = read_u32_le(stream);
    auto file_size_raw = read_u32_le(stream);
    auto file_size_dup = read_u32_le(stream);
    if (!creation_date || !file_size_raw || !file_size_dup) {
      return {{}, Error::kInvalidFormat};
    }
    if (*file_size_raw != *file_size_dup) {
      return {{}, Error::kInvalidFormat};
    }

    char separator{};
    if (!stream.get(separator)) {
      return {{}, Error::kInvalidFormat};
    }
    if (separator != 0x00) {
      return {{}, Error::kInvalidFormat};
    }

    auto file_count = read_u32_le(stream);
    if (!file_count || *file_count == 0) {
      return {{}, Error::kInvalidFormat};
    }

    std::vector<EntryInfo> entries;
    // Sanity check: minimum entry size is 23 bytes (direction + animation +
    // monster + base + nspm + kit + 7 texture count bytes with 0 sprites).
    // file_size_raw is the payload size from fileAmount through end of file.
    // Guard against absurd file_count that would cause reserve() OOM.
    constexpr uint64_t kMinEntrySize = 23;
    if (*file_size_raw < static_cast<uint64_t>(*file_count) * kMinEntrySize) {
      return {{}, Error::kInvalidFormat};
    }
    entries.reserve(*file_count);

    for (uint32_t i = 0; i < *file_count; ++i) {
      // Record the start offset of this entry's data block.
      // We subtract kEntryHeaderSize because NosArchive::read_entry adds it
      // back for non-text entries (the CCINF format has no per-entry header).
      auto data_start = static_cast<uint64_t>(stream.tellg());
      auto stored_offset = data_start - kEntryHeaderSize;

      // Parse entry header
      auto direction_byte = stream.get();
      auto animation_byte = stream.get();
      if (direction_byte == std::char_traits<char>::eof()
          || animation_byte == std::char_traits<char>::eof()) {
        return {{}, Error::kReadError};
      }

      auto monster_opt = read_u16_le(stream);
      if (!monster_opt) {
        return {{}, Error::kReadError};
      }

      auto base = read_u32_le(stream);
      auto nspm = read_u32_le(stream);
      auto kit = read_u32_le(stream);
      if (!base || !nspm || !kit) {
        return {{}, Error::kReadError};
      }

      // Read 7 texture parts
      for (int part = 0; part < 7; ++part) {
        int count = stream.get();
        if (count == std::char_traits<char>::eof()) {
          return {{}, Error::kReadError};
        }
        for (int s = 0; s < count; ++s) {
          auto sprite_idx = read_u16_le(stream);
          auto sprite_id = read_u32_le(stream);
          if (!sprite_idx || !sprite_id) {
            return {{}, Error::kReadError};
          }
        }
      }

      // Compute the actual size of this entry's data
      auto data_end = static_cast<uint64_t>(stream.tellg());
      auto entry_data_size = data_end - data_start;

      auto name = std::to_string(i);

      entries.push_back({
          .id = i,
          .name = std::move(name),
          .creation_date = *creation_date,
          .compressed = false,
          .type = EntryType::Unknown,
          .offset = stored_offset,
          .compressed_size = entry_data_size,
          .uncompressed_size = entry_data_size,
          .codec = nullptr,
      });
    }

    return {std::move(entries)};
  }

}  // namespace onex::archive
