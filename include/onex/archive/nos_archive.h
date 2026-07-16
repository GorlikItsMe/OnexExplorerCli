#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace onex::archive {

  /// Size of the .NOS file header in bytes.
  static constexpr int kNosHeaderSize = 0x10;

  /// Size of a per-entry header in the entry table.
  static constexpr int kEntryHeaderSize = 13;

  /// Represents an open .NOS archive with memory-mapped access.
  ///
  /// Use @ref open to construct an instance.  Once opened, the entry table and
  /// individual entry bodies can be read.  Archives are move-only and release
  /// the memory mapping on destruction.
  class NosArchive {
  public:
    /// The 16-byte raw archive header.
    using Header = std::array<byte, kNosHeaderSize>;

    NosArchive(NosArchive&& other) noexcept;
    auto operator=(NosArchive&& other) noexcept -> NosArchive&;
    NosArchive(const NosArchive&) = delete;
    auto operator=(const NosArchive&) -> NosArchive& = delete;
    ~NosArchive();

    /// Open a .NOS file from disk.  Returns an error if the file cannot be
    /// opened or the header is invalid.
    static auto open(const std::filesystem::path& filepath) -> Result<NosArchive>;

    /// The raw 16-byte archive header.
    auto header() const -> const Header& { return header_; }

    /// The path of the opened file.
    auto filepath() const -> const std::string& { return filepath_; }

    /// Whether this archive has been successfully opened.
    auto is_open() const -> bool { return !filepath_.empty(); }

    /// The parsed entry table — one entry per file in the archive.
    auto entries() const -> std::span<const EntryInfo> { return entries_; }

    /// Read (and decompress) the body of entry @p index.
    auto read_entry(size_t index) -> Result<std::vector<uint8_t>>;

  private:
    NosArchive() = default;

    auto release() -> void;

    Header header_{};
    std::string filepath_;
    void* map_addr_ = nullptr;
    size_t map_size_ = 0;
    std::vector<EntryInfo> entries_;
    std::vector<uint8_t> buf_;
  };

}  // namespace onex::archive
