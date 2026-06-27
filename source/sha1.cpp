#include <onex/sha1.h>

#include <array>
#include <cstring>
#include <format>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace onex {

  namespace {

    constexpr std::array<uint32_t, 4> K = {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6,
    };

    [[nodiscard]] constexpr uint32_t f(int t, uint32_t b, uint32_t c, uint32_t d) {
      if (t < 20) return (b & c) | ((~b) & d);
      if (t < 40) return b ^ c ^ d;
      if (t < 60) return (b & c) | (b & d) | (c & d);
      return b ^ c ^ d;
    }

    [[nodiscard]] constexpr uint32_t rotl(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

  }  // namespace

  Sha1::Sha1()
      : state_{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0},
        buffer_{},
        count_{0},
        finalized_{false} {}

  void Sha1::update(std::string_view data) {
    if (finalized_) throw std::runtime_error("Sha1::update() called after finalization");

    for (size_t i = 0; i < data.size(); ++i) {
      buffer_[count_ % 64] = static_cast<std::byte>(data[i]);
      ++count_;
      if (count_ % 64 == 0) {
        process_block(buffer_);
      }
    }
  }

  void Sha1::process_block(const std::array<std::byte, 64>& block) {
    std::array<uint32_t, 80> w{};

    for (int t = 0; t < 16; ++t) {
      w[t] = (static_cast<uint32_t>(block[t * 4]) << 24)
             | (static_cast<uint32_t>(block[t * 4 + 1]) << 16)
             | (static_cast<uint32_t>(block[t * 4 + 2]) << 8)
             | static_cast<uint32_t>(block[t * 4 + 3]);
    }
    for (int t = 16; t < 80; ++t) {
      w[t] = rotl(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];

    for (int t = 0; t < 80; ++t) {
      uint32_t temp = rotl(a, 5) + f(t, b, c, d) + e + K[t / 20] + w[t];
      e = d;
      d = c;
      c = rotl(b, 30);
      b = a;
      a = temp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
  }

  void Sha1::finalize() {
    if (finalized_) return;

    uint64_t bits = count_ * 8;

    std::byte pad_byte{0x80};
    update(std::string_view{reinterpret_cast<const char*>(&pad_byte), 1});

    while (count_ % 64 != 56) {
      std::byte zero{};
      update(std::string_view{reinterpret_cast<const char*>(&zero), 1});
    }

    for (int i = 7; i >= 0; --i) {
      std::byte b{static_cast<std::byte>((bits >> (i * 8)) & 0xFF)};
      update(std::string_view{reinterpret_cast<const char*>(&b), 1});
    }

    finalized_ = true;
  }

  std::string Sha1::hex_digest() {
    finalize();

    std::ostringstream oss;
    for (auto s : state_) {
      oss << std::hex << std::setfill('0') << std::setw(8) << s;
    }
    return oss.str();
  }

  std::array<std::byte, 20> Sha1::digest() {
    finalize();

    std::array<std::byte, 20> result{};
    for (int i = 0; i < 5; ++i) {
      result[i * 4] = static_cast<std::byte>((state_[i] >> 24) & 0xFF);
      result[i * 4 + 1] = static_cast<std::byte>((state_[i] >> 16) & 0xFF);
      result[i * 4 + 2] = static_cast<std::byte>((state_[i] >> 8) & 0xFF);
      result[i * 4 + 3] = static_cast<std::byte>(state_[i] & 0xFF);
    }
    return result;
  }

  std::string Sha1::file_hex_digest(std::string_view path) {
    Sha1 sha1;
    std::ifstream file(std::string{path}, std::ios::binary);
    if (!file) throw std::runtime_error(std::format("failed to open '{}' for SHA1 hashing", path));

    std::array<char, 8192> buffer{};
    while (file) {
      file.read(buffer.data(), buffer.size());
      if (auto gcount = file.gcount(); gcount > 0) {
        sha1.update(std::string_view{buffer.data(), static_cast<size_t>(gcount)});
      }
    }

    return sha1.hex_digest();
  }

}  // namespace onex
