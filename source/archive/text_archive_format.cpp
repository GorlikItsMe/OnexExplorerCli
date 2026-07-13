#include <onex/archive/text_archive_format.h>
#include <onex/archive/text_dat_codec.h>
#include <onex/archive/text_lst_codec.h>

#include <array>
#include <istream>
#include <optional>
#include <string>
#include <vector>

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

  }  // namespace

  auto TextArchiveFormat::parse_entry_table(std::span<const uint8_t> /*header*/,
                                            std::istream& stream)
      -> Result<std::vector<EntryInfo>> {
    // TextArchive has no unique header magic — seek back to start
    stream.seekg(0);
    if (!stream) {
      return {{}, Error::kReadError};
    }

    auto file_count = read_u32_le(stream);
    if (!file_count) {
      return {{}, Error::kInvalidFormat};
    }

    // Sanity check: each entry needs at least 16 bytes (file_number 4B +
    // name_length 4B + is_dat 4B + data_size 4B). If the claimed count
    // couldn't possibly fit in the remaining stream, reject early instead of
    // allocating an absurd amount of memory (prevents std::bad_alloc on
    // non-NOS files that fall through to this format).
    auto pos = stream.tellg();
    if (pos != -1) {
      stream.seekg(0, std::ios::end);
      auto remaining = static_cast<uint64_t>(stream.tellg()) - static_cast<uint64_t>(pos);
      stream.seekg(pos);
      // Absolute minimum overhead: 16 bytes per entry (header fields only,
      // zero-length name and data). If the count exceeds this, the file is
      // clearly not a valid text-format archive.
      constexpr uint32_t kMinEntryOverhead = 16;
      if (*file_count > remaining / kMinEntryOverhead) {
        return {{}, Error::kInvalidFormat};
      }
    }

    std::vector<EntryInfo> entries;
    entries.reserve(*file_count);

    for (uint32_t i = 0; i < *file_count; ++i) {
      auto file_number = read_u32_le(stream);
      auto name_length = read_u32_le(stream);
      if (!file_number || !name_length) {
        return {{}, Error::kInvalidFormat};
      }

      std::string name(*name_length, '\0');
      if (!stream.read(name.data(), *name_length)) {
        return {{}, Error::kReadError};
      }

      auto is_dat = read_u32_le(stream);
      auto data_size = read_u32_le(stream);
      if (!is_dat || !data_size) {
        return {{}, Error::kInvalidFormat};
      }

      // Record the offset of the data block (current stream position)
      auto data_offset = static_cast<uint64_t>(stream.tellg());

      // Skip past the data
      stream.seekg(*data_size, std::ios::cur);
      if (!stream) {
        return {{}, Error::kReadError};
      }

      bool is_dat_file = *is_dat != 0 || name.ends_with(".dat");

      entries.push_back({
          .id = *file_number,
          .name = std::move(name),
          .creation_date = 0,
          .compressed = false,
          .type = is_dat_file ? EntryType::TextDat : EntryType::TextLst,
          .offset = data_offset,
          .compressed_size = *data_size,
          .uncompressed_size = *data_size,
          .codec = is_dat_file ? text_dat_codec() : text_lst_codec(),
      });
    }

    return {std::move(entries)};
  }

}  // namespace onex::archive
