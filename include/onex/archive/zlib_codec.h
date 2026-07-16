#pragma once

#include <onex/archive/codec.h>

namespace onex::archive {

  /// Returns the singleton zlib (deflate/inflate) codec.
  extern const Codec* zlib_codec();

}  // namespace onex::archive