#pragma once

#include <onex/core/error.h>

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <span>

namespace onex::archive {

  /// CCINF V1.20 sprite entries come in two variants that share the
  /// 16-byte header + 7 texture-part lists layout but differ in the meaning
  /// of header bytes 0x02/0x03 and in the part list keys.
  ///
  /// The header follows the OnexExplorer reference interpretation (direction,
  /// animation, monster | sex/class+morph, base, nspm, kit), verified
  /// byte-level against the current CDN build — see
  /// research/ccinf-entry-fields.md. taletool's alternative reading
  /// (id/base/remap/animation) is a documented discrepancy, not the schema.
  enum class SpriteVariant {
    kMonster,  ///< NSmnData: monster(u16) at 0x02, parts keyed "0".."6"
    kPlayer,   ///< NSpnData: sex/class (packed byte) + morph, named parts
  };

  /// Decodes a CCINF V1.20 sprite info entry (16-byte header + 7 counted
  /// texture-part lists) into JSON. Malformed input yields kInvalidFormat.
  auto decode_sprite_info(std::span<const uint8_t> data, SpriteVariant variant)
      -> Result<nlohmann::json>;

}  // namespace onex::archive
