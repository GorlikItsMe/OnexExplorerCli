#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/nos_archive.h>

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
    result.stream_ = std::move(file);
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
  }

  auto NosArchive::read_entry(size_t index) -> Result<std::vector<uint8_t>> {
    if (index >= entries_.size()) {
      return {{}, Error::kEntryNotFound};
    }

    const auto& entry = entries_[index];

    auto data_offset = static_cast<std::streamoff>(entry.offset) + kEntryHeaderSize;
    stream_.seekg(data_offset);
    if (!stream_) {
      return {{}, Error::kReadError};
    }

    std::vector<uint8_t> compressed(entry.compressed_size);
    if (!stream_.read(reinterpret_cast<char*>(compressed.data()),
                      static_cast<std::streamsize>(entry.compressed_size))) {
      return {{}, Error::kReadError};
    }

    if (!entry.compressed) {
      return {std::move(compressed)};
    }

    auto decompressed = entry.codec->decode(compressed);
    if (!decompressed) {
      return {std::move(decompressed.value), decompressed.error};
    }

    if (decompressed.value.size() != entry.uncompressed_size) {
      return {{}, Error::kCorruptArchive};
    }

    return {std::move(decompressed.value)};
  }

}  // namespace onex::archive
