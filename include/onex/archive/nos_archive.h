#pragma once

#include <onex/archive/entry.h>
#include <onex/core/error.h>

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace onex::archive {

  static constexpr int kNosHeaderSize = 0x10;

  class NosArchive {
  public:
    using Header = std::array<byte, kNosHeaderSize>;

    static auto open(const std::filesystem::path& filepath) -> Result<NosArchive>;

    auto header() const -> const Header& { return header_; }
    auto filepath() const -> const std::string& { return filepath_; }
    auto is_open() const -> bool { return is_open_; }
    auto entries() const -> std::span<const EntryInfo> { return entries_; }

  private:
    NosArchive() = default;

    Header header_{};
    std::string filepath_;
    bool is_open_ = false;
    std::vector<EntryInfo> entries_;
  };

}  // namespace onex::archive
