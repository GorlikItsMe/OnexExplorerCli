#pragma once

// Internal helpers shared across the per-format converter translation units.
// This header lives in source/ and is not installed.

#include <cstddef>
#include <cstdint>

namespace onex::archive::detail {

  // 4-bit nibble (0..15) expanded to an 8-bit channel (0, 17, ..., 255).
  constexpr auto nibble_to_channel(uint8_t nibble) -> uint8_t {
    return static_cast<uint8_t>(nibble * 0x11);
  }

  // 8-bit channel (0..255) quantised to a 4-bit nibble (0..15).
  constexpr auto channel_to_nibble(uint8_t channel) -> uint8_t {
    return static_cast<uint8_t>(channel / 0x11);
  }

  // 5-bit field (0..31) expanded to an 8-bit channel (0, 8, ..., 248).
  constexpr auto five_to_channel(uint8_t bits) -> uint8_t { return static_cast<uint8_t>(bits * 8); }

  // 8-bit channel (0..255) quantised to a 5-bit field (0..31).
  constexpr auto channel_to_five(uint8_t channel) -> uint8_t {
    return static_cast<uint8_t>(channel / 8);
  }

  // Total pixel count for a width x height image, as a size_t to avoid
  // signed-overflow on large images.
  inline auto pixel_count(int width, int height) -> size_t {
    return static_cast<size_t>(width) * static_cast<size_t>(height);
  }

}  // namespace onex::archive::detail
