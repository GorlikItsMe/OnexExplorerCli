#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/nos_archive.h>

#include <fstream>
#include <vector>

namespace onex::archive {

  auto NosArchive::open(const std::filesystem::path& filepath) -> Result<NosArchive> {
    if (!std::filesystem::exists(filepath)) {
      return {NosArchive{}, Error::kFileNotFound};
    }

    std::ifstream file{filepath, std::ios::binary};
    if (!file.is_open()) {
      return {NosArchive{}, Error::kReadError};
    }

    Header header{};
    if (!file.read(reinterpret_cast<char*>(header.data()), kNosHeaderSize)) {
      return {NosArchive{}, Error::kReadError};
    }

    auto format = ArchiveFormat::detect(header);
    if (!format) {
      return {NosArchive{}, Error::kInvalidFormat};
    }

    auto entries = format->parse_entry_table(header, file);
    if (!entries) {
      return {NosArchive{}, entries.error};
    }

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
  }

  auto NosArchive::ensure_loaded() -> Error {
    if (!data_.empty()) {
      return Error::kNone;
    }

    std::ifstream file{filepath_, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
      return Error::kReadError;
    }

    auto file_size = file.tellg();
    if (file_size < kNosHeaderSize) {
      return Error::kInvalidFormat;
    }

    data_.resize(static_cast<size_t>(file_size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(data_.data()), file_size)) {
      data_.clear();
      return Error::kReadError;
    }

    return Error::kNone;
  }

  auto NosArchive::read_entry(size_t index) -> Result<std::vector<uint8_t>> {
    if (index >= entries_.size()) {
      return {{}, Error::kEntryNotFound};
    }

    auto err = ensure_loaded();
    if (err != Error::kNone) {
      return {{}, err};
    }

    const auto& entry = entries_[index];

    bool is_text = entry.type == EntryType::TextDat || entry.type == EntryType::TextLst;
    auto data_offset = static_cast<size_t>(entry.offset) + (is_text ? 0 : kEntryHeaderSize);
    if (data_offset + entry.compressed_size > data_.size()) {
      return {{}, Error::kCorruptArchive};
    }

    std::span<const uint8_t> compressed{data_.data() + data_offset,
                                        static_cast<size_t>(entry.compressed_size)};

    bool needs_decode = entry.compressed || is_text;
    if (needs_decode && entry.codec) {
      auto decompressed = entry.codec->decode(compressed);
      if (!decompressed) {
        return {std::move(decompressed.value), decompressed.error};
      }

      if (entry.compressed && decompressed.value.size() != entry.uncompressed_size) {
        return {{}, Error::kCorruptArchive};
      }

      return {std::move(decompressed.value)};
    }

    return {std::vector<uint8_t>{compressed.begin(), compressed.end()}};
  }

}  // namespace onex::archive
