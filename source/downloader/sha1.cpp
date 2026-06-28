#include <onex/downloader/sha1.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace onex::downloader {

  namespace {

    struct Sha1Ctx {
      std::uint32_t state[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
      std::uint64_t count = 0;
      std::array<std::uint8_t, 64> buf{};
    };

    auto rotl(std::uint32_t x, std::uint32_t n) -> std::uint32_t {
      return (x << n) | (x >> (32 - n));
    }

    void sha1_transform(Sha1Ctx& ctx, const std::uint8_t block[64]) {
      std::uint32_t w[80];
      for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24)
               | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16)
               | (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8)
               | (static_cast<std::uint32_t>(block[i * 4 + 3]));
      }
      for (int i = 16; i < 80; ++i) {
        w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
      }

      auto a = ctx.state[0];
      auto b = ctx.state[1];
      auto c = ctx.state[2];
      auto d = ctx.state[3];
      auto e = ctx.state[4];

      for (int i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20) {
          f = (b & c) | (~b & d);
          k = 0x5A827999;
        } else if (i < 40) {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
        } else if (i < 60) {
          f = (b & c) | (b & d) | (c & d);
          k = 0x8F1BBCDC;
        } else {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
        }
        auto tmp = rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = tmp;
      }

      ctx.state[0] += a;
      ctx.state[1] += b;
      ctx.state[2] += c;
      ctx.state[3] += d;
      ctx.state[4] += e;
    }

    void sha1_update(Sha1Ctx& ctx, const std::uint8_t* data, std::size_t len) {
      auto idx = static_cast<std::size_t>(ctx.count & 63);
      ctx.count += static_cast<std::uint64_t>(len);
      auto fill = 64 - idx;

      if (len >= fill) {
        std::memcpy(ctx.buf.data() + idx, data, fill);
        sha1_transform(ctx, ctx.buf.data());
        data += fill;
        len -= fill;
        while (len >= 64) {
          sha1_transform(ctx, data);
          data += 64;
          len -= 64;
        }
        idx = 0;
      }
      std::memcpy(ctx.buf.data() + idx, data, len);
    }

    auto sha1_final(Sha1Ctx& ctx) -> std::array<std::uint8_t, 20> {
      auto bits = ctx.count * 8;
      auto idx = static_cast<std::size_t>(ctx.count & 63);

      ctx.buf[idx++] = 0x80;
      if (idx > 56) {
        std::memset(ctx.buf.data() + idx, 0, 64 - idx);
        sha1_transform(ctx, ctx.buf.data());
        idx = 0;
      }
      std::memset(ctx.buf.data() + idx, 0, 56 - idx);

      for (int i = 0; i < 8; ++i) {
        ctx.buf[56 + i] = static_cast<std::uint8_t>(bits >> (56 - i * 8));
      }
      sha1_transform(ctx, ctx.buf.data());

      std::array<std::uint8_t, 20> hash{};
      for (int i = 0; i < 5; ++i) {
        hash[i * 4] = static_cast<std::uint8_t>(ctx.state[i] >> 24);
        hash[i * 4 + 1] = static_cast<std::uint8_t>(ctx.state[i] >> 16);
        hash[i * 4 + 2] = static_cast<std::uint8_t>(ctx.state[i] >> 8);
        hash[i * 4 + 3] = static_cast<std::uint8_t>(ctx.state[i]);
      }
      return hash;
    }

    auto to_hex(const std::array<std::uint8_t, 20>& hash) -> std::string {
      std::ostringstream oss;
      for (auto b : hash) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
      }
      return oss.str();
    }

  }  // anonymous namespace

  auto sha1_hex(const std::vector<byte>& data) -> std::string {
    Sha1Ctx ctx;
    sha1_update(ctx, data.data(), data.size());
    return to_hex(sha1_final(ctx));
  }

  auto sha1_file_hex(const std::string& path) -> std::string {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      return {};
    }

    Sha1Ctx ctx;
    std::array<std::uint8_t, 65536> buf{};
    while (file.read(reinterpret_cast<char*>(buf.data()), buf.size()) || file.gcount() > 0) {
      sha1_update(ctx, buf.data(), static_cast<std::size_t>(file.gcount()));
    }

    return to_hex(sha1_final(ctx));
  }

}  // namespace onex::downloader
