#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <span>

namespace onex::archive {

  /// Decode a decompressed NSeffData entry body (effect definition) into a JSON
  /// document.
  ///
  /// The body holds a 24-byte root header, 192-byte component records (all
  /// fields, including the kind-specific unions positioned/flying/particle),
  /// and variable track data (key times and values). Layout per taletool
  /// `docs/formats/effects.md`; field names and nesting follow the R2 JSON
  /// schema prototype (#48).
  ///
  /// @param data The full decompressed entry body.
  /// @return A JSON document on success, or kInvalidFormat if the body is
  ///         truncated or its records are inconsistent.
  auto decode_effect_definition(std::span<const uint8_t> data) -> Result<nlohmann::json>;

}  // namespace onex::archive
