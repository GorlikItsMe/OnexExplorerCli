#pragma once

#include <onex/archive/codec.h>

namespace onex::archive {

  /// Returns the singleton codec for .dat text entries (NosTale XOR-based).
  extern const Codec* text_dat_codec();

}  // namespace onex::archive
