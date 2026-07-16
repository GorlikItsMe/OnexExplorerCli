#include <onex/archive/archive_format.h>
#include <onex/archive/codec.h>
#include <onex/archive/nos_archive.h>

#include <fstream>
#include <vector>

#ifdef _WIN32
#  include <io.h>
#else
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <unistd.h>
#endif

namespace onex::archive {

  NosArchive::NosArchive(NosArchive&& other) noexcept
      : header_(other.header_),
        filepath_(std::move(other.filepath_)),
        map_addr_(other.map_addr_),
        map_size_(other.map_size_),
        entries_(std::move(other.entries_)),
        buf_(std::move(other.buf_)) {
    other.map_addr_ = nullptr;
    other.map_size_ = 0;
  }

  auto NosArchive::operator=(NosArchive&& other) noexcept -> NosArchive& {
    if (this != &other) {
      release();
      header_ = other.header_;
      filepath_ = std::move(other.filepath_);
      map_addr_ = other.map_addr_;
      map_size_ = other.map_size_;
      entries_ = std::move(other.entries_);
      buf_ = std::move(other.buf_);
      other.map_addr_ = nullptr;
      other.map_size_ = 0;
    }
    return *this;
  }

  NosArchive::~NosArchive() { release(); }

  auto NosArchive::release() -> void {
    if (map_addr_) {
#ifndef _WIN32
      ::munmap(map_addr_, map_size_);
#endif
      map_addr_ = nullptr;
      map_size_ = 0;
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

    // Get file size and map/read the entire file into memory so
    // read_entry() can serve slices without per-entry I/O.
    file.seekg(0, std::ios::end);
    auto file_size = file.tellg();
    if (file_size == -1) {
      return {NosArchive{}, Error::kReadError};
    }

    auto size = static_cast<size_t>(file_size);

#ifdef _WIN32
    std::vector<uint8_t> buf(size);
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(size))) {
      return {NosArchive{}, Error::kReadError};
    }

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.map_addr_ = buf.data();
    result.map_size_ = size;
    result.buf_ = std::move(buf);
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
#else
    int fd = ::open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
      return {NosArchive{}, Error::kReadError};
    }

    void* addr = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (addr == MAP_FAILED) {
      return {NosArchive{}, Error::kReadError};
    }

    ::posix_madvise(addr, size, POSIX_MADV_WILLNEED);
    ::posix_madvise(addr, size, POSIX_MADV_SEQUENTIAL);

    NosArchive result;
    result.header_ = header;
    result.filepath_ = filepath.string();
    result.map_addr_ = addr;
    result.map_size_ = size;
    result.entries_ = std::move(entries.value);
    return {std::move(result)};
#endif
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
