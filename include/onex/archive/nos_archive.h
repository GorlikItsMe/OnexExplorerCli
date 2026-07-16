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

  static constexpr int kNosHeaderSize = 0x10;
  static constexpr int kEntryHeaderSize = 13;

  class NosArchive {
  public:
    using Header = std::array<byte, kNosHeaderSize>;

    NosArchive(NosArchive&& other) noexcept;
    auto operator=(NosArchive&& other) noexcept -> NosArchive&;
    NosArchive(const NosArchive&) = delete;
    auto operator=(const NosArchive&) -> NosArchive& = delete;
    ~NosArchive();

    static auto open(const std::filesystem::path& filepath) -> Result<NosArchive>;

    auto header() const -> const Header& { return header_; }
    auto filepath() const -> const std::string& { return filepath_; }
    auto is_open() const -> bool { return !filepath_.empty(); }
    auto entries() const -> std::span<const EntryInfo> { return entries_; }

    auto read_entry(size_t index) -> Result<std::vector<uint8_t>>;

  private:
    NosArchive() = default;

    auto ensure_loaded() -> Error;

    Header header_{};
    std::string filepath_;
    void* map_addr_ = nullptr;
    size_t map_size_ = 0;
    std::vector<EntryInfo> entries_;
  };

}  // namespace onex::archive
