#pragma once

#include <cstdint>

/// Fundamental type definitions and error handling for the onex library.
namespace onex {

  /// Raw byte type used throughout the library.
  using byte = std::uint8_t;

  /// Error codes returned by most library operations.
  enum class Error {
    kNone,              ///< No error; the operation succeeded.
    kInvalidHeader,     ///< The archive header is missing or malformed.
    kInvalidFormat,     ///< The archive format is not recognised.
    kCorruptArchive,    ///< The archive data is corrupt or truncated.
    kEntryNotFound,     ///< A requested entry was not found in the archive.
    kAmbiguousMatch,    ///< More than one entry matched the lookup criteria.
    kDecompressFailed,  ///< Decompression of entry data failed.
    kCompressFailed,    ///< Compression of entry data failed.
    kFileNotFound,      ///< The specified file does not exist.
    kReadError,         ///< An I/O read operation failed.
    kIoError,           ///< A general I/O error occurred.
    kNetworkError,      ///< A network request failed.
  };

  /// Holds a value of type T together with an optional error code.
  /// Implicitly convertible to bool — @c true means success.
  template <typename T> struct Result {
    T value;                     ///< The result value (valid only when error is kNone).
    Error error = Error::kNone;  ///< Error status of the operation.

    /// Returns @c true when the operation succeeded.
    explicit operator bool() const { return error == Error::kNone; }
  };

}  // namespace onex
