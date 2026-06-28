#pragma once

#include <onex/core/error.h>

#include <array>
#include <filesystem>
#include <string>

namespace onex::archive {

  static constexpr int kNosHeaderSize = 0x10;

  class NosArchive {
  public:
    using Header = std::array<byte, kNosHeaderSize>;

    static auto open(const std::filesystem::path& filepath) -> Result<NosArchive>;

    auto header() const -> const Header& { return header_; }
    auto filepath() const -> const std::string& { return filepath_; }
    auto is_open() const -> bool { return is_open_; }

  private:
    NosArchive() = default;

    Header header_{};
    std::string filepath_;
    bool is_open_ = false;
  };

}  // namespace onex::archive
