#include <fcntl.h>
#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/nos_archive.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fstream>
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
    if (map_addr_) {
      return Error::kNone;
    }

    int fd = ::open(filepath_.c_str(), O_RDONLY);
    if (fd == -1) {
      return Error::kReadError;
    }

    off_t file_size = ::lseek(fd, 0, SEEK_END);
    if (file_size < kNosHeaderSize) {
      ::close(fd);
      return Error::kInvalidFormat;
    }

    auto size = static_cast<size_t>(file_size);
    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
      ::close(fd);
      return Error::kReadError;
    }

    ::close(fd);

    ::posix_madvise(addr, size, POSIX_MADV_WILLNEED);

    map_addr_ = addr;
    map_size_ = size;
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
    if (data_offset + entry.compressed_size > map_size_) {
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
