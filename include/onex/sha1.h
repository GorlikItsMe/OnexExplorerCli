#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace onex {

  class Sha1 {
  public:
    Sha1();

    void update(std::string_view data);

    [[nodiscard]] std::string hex_digest();
    [[nodiscard]] std::array<std::byte, 20> digest();

    static std::string file_hex_digest(std::string_view path);

  private:
    void process_block(const std::array<std::byte, 64>& block);
    void finalize();

    std::array<uint32_t, 5> state_;
    std::array<std::byte, 64> buffer_;
    uint64_t count_;
    bool finalized_;
  };

}  // namespace onex
