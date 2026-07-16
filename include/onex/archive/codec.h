#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <span>
#include <vector>

namespace onex::archive {

  /// Abstract codec for compressing/decompressing entry bodies.
  struct Codec {
    virtual ~Codec() = default;

    /// Decompress or decrypt @p input into raw bytes.
    virtual auto decode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> = 0;

    /// Compress or encrypt @p input into the archive storage format.
    virtual auto encode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> = 0;
  };

}  // namespace onex::archive
