#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  struct Codec {
    virtual ~Codec() = default;
    virtual auto decode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> = 0;
    virtual auto encode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> = 0;
  };

}  // namespace onex::archive
