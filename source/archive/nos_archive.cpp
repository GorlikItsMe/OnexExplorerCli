#include <fcntl.h>
#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/nos_archive.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <spanstream>
#include <vector>

namespace onex::archive {

  NosArchive::NosArchive(NosArchive&& other) noexcept
      : header_(other.header_),
        filepath_(std::move(other.filepath_)),
        map_addr_(other.map_addr_),
        map_size_(other.map_size_),
        entries_(std::move(other.entries_)) {
    other.map_addr_ = nullptr;
    other.map_size_ = 0;
  }

  auto NosArchive::operator=(NosArchive&& other) noexcept -> NosArchive& {
    if (this != &other) {
      if (map_addr_) {
        ::munmap(map_addr_, map_size_);
      }
      header_ = other.header_;
      filepath_ = std::move(other.filepath_);
      map_addr_ = other.map_addr_;
      map_size_ = other.map_size_;
      entries_ = std::move(other.entries_);
      other.map_addr_ = nullptr;
      other.map_size_ = 0;
    }
    return *this;
  }

  NosArchive::~NosArchive() {
    if (map_addr_) {
      ::munmap(map_addr_, map_size_);
    }
  }

  auto NosArchive::open(const std::filesystem::path& filepath) -> Result<NosArchive> {
    if (!std::filesystem::exists(filepath)) {
      return {NosArchive{}, Error::kFileNotFound};
    }

    int fd = ::open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
      return {NosArchive{}, Error::kReadError};
    }

    off_t file_size = ::lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
      ::close(fd);
      return {NosArchive{}, Error::kReadError};
    }
    if (file_size < kNosHeaderSize) {
      ::close(fd);
      return {NosArchive{}, Error::kInvalidFormat};
    }

    auto size = static_cast<size_t>(file_size);
    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) {
      return {NosArchive{}, Error::kReadError};
    }

    ::posix_madvise(addr, size, POSIX_MADV_WILLNEED);
    ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL);

    Header header;
    std::memcpy(header.data(), addr, kNosHeaderSize);

    auto format = ArchiveFormat::detect(header);
    if (!format) {
      ::munmap(addr, size);
      return {NosArchive{}, Error::kInvalidFormat};
    }

    std::ispanstream stream{std::span<const char>{static_cast<const char*>(addr), size}};
    stream.seekg(kNosHeaderSize);

    auto entries = format->parse_entry_table(header, stream);
    if (!entries) {
      ::munmap(addr, size);
      return {NosArchive{}, entries.error};
    }

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.map_addr_ = addr;
    result.map_size_ = size;
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
  }

  auto NosArchive::read_entry(size_t index) -> Result<std::vector<uint8_t>> {
    if (index >= entries_.size()) {
      return {{}, Error::kEntryNotFound};
    }

    const auto& entry = entries_[index];

    bool is_text = entry.type == EntryType::TextDat || entry.type == EntryType::TextLst;
    auto data_offset = static_cast<size_t>(entry.offset) + (is_text ? 0 : kEntryHeaderSize);
    if (data_offset > map_size_ || entry.compressed_size > map_size_ - data_offset) {
      return {{}, Error::kCorruptArchive};
    }

    auto base = static_cast<const uint8_t*>(map_addr_);
    std::span<const uint8_t> compressed{base + data_offset,
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
