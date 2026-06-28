#pragma once

#include <cstdint>

namespace onex {

  using byte = std::uint8_t;

  enum class Error {
    kNone,
    kInvalidHeader,
    kCorruptArchive,
    kEntryNotFound,
    kAmbiguousMatch,
    kDecompressFailed,
    kCompressFailed,
    kIoError,
    kNetworkError,
  };

  template <typename T> struct Result {
    T value;
    Error error = Error::kNone;

    explicit operator bool() const { return error == Error::kNone; }
  };

}  // namespace onex
