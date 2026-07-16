#pragma once

#include <onex/archive/codec.h>

namespace onex::archive {

  /// Returns the singleton codec for .lst text entries.
  extern const Codec* text_lst_codec();

}  // namespace onex::archive
