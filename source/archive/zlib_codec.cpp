#include <onex/archive/zlib_codec.h>
#include <zlib.h>

#include <vector>

namespace onex::archive {

  namespace {

    struct ZlibCodec final : Codec {
      auto decode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        if (input.empty()) {
          return {std::vector<uint8_t>{}};
        }

        // We don't know uncompressed_size here; use inflate for streaming.
        // Use a reasonable initial buffer and grow as needed.
        z_stream strm{};
        strm.next_in = const_cast<uint8_t*>(input.data());
        strm.avail_in = static_cast<uInt>(input.size());

        auto ret = inflateInit(&strm);
        if (ret != Z_OK) {
          return {{}, Error::kDecompressFailed};
        }

        std::vector<uint8_t> output;
        output.reserve(input.size() * 4);  // heuristic
        constexpr size_t kChunk = 65536;

        do {
          if (strm.avail_out == 0) {
            auto old_size = output.size();
            output.resize(old_size + kChunk);
            strm.next_out = output.data() + old_size;
            strm.avail_out = kChunk;
          }
          ret = inflate(&strm, Z_NO_FLUSH);
          if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&strm);
            return {{}, Error::kDecompressFailed};
          }
        } while (ret != Z_STREAM_END);

        output.resize(strm.total_out);
        inflateEnd(&strm);
        return {std::move(output)};
      }

      auto encode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        auto bound = compressBound(static_cast<uLong>(input.size()));
        std::vector<uint8_t> output(bound);

        uLongf dest_len = bound;
        auto ret = compress2(output.data(), &dest_len, input.data(),
                             static_cast<uLong>(input.size()), Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK) {
          return {{}, Error::kCompressFailed};
        }

        output.resize(dest_len);
        return {std::move(output)};
      }
    };

    const ZlibCodec kZlibCodec;

  }  // namespace

  auto zlib_codec() -> const Codec* { return &kZlibCodec; }

}  // namespace onex::archive