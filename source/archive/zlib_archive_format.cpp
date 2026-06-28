#include <onex/archive/zlib_archive_format.h>
#include <onex/archive/zlib_codec.h>

#include <array>
#include <optional>
#include <sstream>

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

    auto contains_name(const std::vector<EntryInfo>& entries, const std::string& name) -> bool {
      for (const auto& e : entries) {
        if (e.name == name) return true;
      }
      return false;
    }

  }  // namespace

  auto ZlibArchiveFormat::parse_entry_table(std::span<const uint8_t> header, std::istream& stream)
      -> Result<std::vector<EntryInfo>> {
    auto sub_type = detect_sub_type(header);

    auto file_count_raw = read_u32_le(stream);
    if (!file_count_raw) {
      return {{}, Error::kInvalidFormat};
    }
    auto file_count = *file_count_raw;

    char separator{};
    if (!stream.get(separator)) {
      return {{}, Error::kInvalidFormat};
    }

    std::vector<EntryInfo> entries;
    entries.reserve(file_count);

    for (uint32_t i = 0; i < file_count; ++i) {
      auto id_raw = read_u32_le(stream);
      auto offset_raw = read_u32_le(stream);
      if (!id_raw || !offset_raw) {
        return {{}, Error::kInvalidFormat};
      }

      auto offset_table_pos = stream.tellg();

      auto abs_offset = static_cast<uint32_t>(*offset_raw);
      stream.seekg(abs_offset);
      if (!stream) {
        return {{}, Error::kReadError};
      }

      auto creation_date = read_u32_le(stream);
      auto data_size = read_u32_le(stream);
      auto compressed_data_size = read_u32_le(stream);
      if (!creation_date || !data_size || !compressed_data_size) {
        return {{}, Error::kReadError};
      }

      int is_compressed_byte = stream.get();
      if (is_compressed_byte == std::char_traits<char>::eof()) {
        return {{}, Error::kReadError};
      }
      bool is_compressed = static_cast<bool>(is_compressed_byte);

      stream.seekg(offset_table_pos);
      if (!stream) {
        return {{}, Error::kReadError};
      }

      auto name = std::to_string(*id_raw);
      if (contains_name(entries, name)) {
        auto suffix = 2u;
        while (contains_name(entries, name + "_" + std::to_string(suffix))) {
          ++suffix;
        }
        name += "_" + std::to_string(suffix);
      }

      entries.push_back({
          .id = *id_raw,
          .name = std::move(name),
          .creation_date = *creation_date,
          .compressed = is_compressed,
          .type = sub_type_to_entry_type(sub_type),
          .offset = abs_offset,
          .compressed_size = *compressed_data_size,
          .uncompressed_size = *data_size,
          .codec = zlib_codec(),
      });
    }

    return {std::move(entries)};
  }

  auto ZlibArchiveFormat::detect_sub_type(std::span<const uint8_t> header) -> int {
    if (header.size() >= 7) {
      if (std::equal(header.begin(), header.begin() + 7, "NT Data")) {
        if (header.size() >= 10) {
          std::array<char, 3> buf{static_cast<char>(header[8]), static_cast<char>(header[9]), '\0'};
          return std::atoi(buf.data());
        }
        return 199;
      }
    }
    if (header.size() >= 10) {
      if (std::equal(header.begin(), header.begin() + 10, "32GBS V1.0")) {
        return 101;  // NS4BbData
      }
      if (std::equal(header.begin(), header.begin() + 10, "ITEMS V1.0")) {
        return 103;  // NSipData2006
      }
    }
    return 199;
  }

  auto ZlibArchiveFormat::sub_type_to_entry_type(int sub_type) -> EntryType {
    switch (sub_type) {
      case 7:   // NStpData
      case 11:  // NStpeData
      case 12:  // NStpuData
        return EntryType::Texture;
      case 24:   // NSipData
      case 103:  // NSipData2006 (ITEMS V1.0)
        return EntryType::Icon;
      case 101:  // NS4BbData (32GBS V1.0)
        return EntryType::Image4B;
      case 5:  // NStcData
        return EntryType::TileGrid;
      default:
        return EntryType::Unknown;
    }
  }

}  // namespace onex::archive
