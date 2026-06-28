#pragma once

#include <cstddef>
#include <cstdint>

namespace onex {

  using byte = std::uint8_t;

  template <typename T> class Span {
  public:
    Span() : data_(nullptr), size_(0) {}
    Span(T* data, std::size_t size) : data_(data), size_(size) {}

    auto data() const -> T* { return data_; }
    auto size() const -> std::size_t { return size_; }
    auto empty() const -> bool { return size_ == 0; }

    auto begin() const -> T* { return data_; }
    auto end() const -> T* { return data_ + size_; }

    auto operator[](std::size_t i) const -> T& { return data_[i]; }

    auto subspan(std::size_t offset, std::size_t count) const -> Span {
      return Span(data_ + offset, count);
    }

  private:
    T* data_;
    std::size_t size_;
  };

  using ByteSpan = Span<const byte>;
  using MutableByteSpan = Span<byte>;

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
