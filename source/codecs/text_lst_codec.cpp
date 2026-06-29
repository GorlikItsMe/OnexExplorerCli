#include <onex/archive/text_lst_codec.h>

#include <array>
#include <cstdint>
#include <vector>

namespace onex::archive {

  namespace {

    auto write_u32_le(std::vector<uint8_t>& buf, uint32_t value) -> void {
      buf.push_back(static_cast<uint8_t>(value & 0xFF));
      buf.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
      buf.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
      buf.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    auto read_u32_le(std::span<const uint8_t> data, size_t& pos) -> uint32_t {
      if (pos + 4 > data.size()) {
        return 0;
      }
      uint32_t value = static_cast<uint32_t>(data[pos]) | static_cast<uint32_t>(data[pos + 1]) << 8
                       | static_cast<uint32_t>(data[pos + 2]) << 16
                       | static_cast<uint32_t>(data[pos + 3]) << 24;
      pos += 4;
      return value;
    }

    struct TextLstCodec final : Codec {
      auto decode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        std::vector<uint8_t> output;
        size_t pos = 0;

        auto line_count = read_u32_le(input, pos);

        for (uint32_t i = 0; i < line_count; ++i) {
          auto line_len = read_u32_le(input, pos);
          if (pos + line_len > input.size()) {
            return {{}, Error::kCorruptArchive};
          }
          for (uint32_t j = 0; j < line_len; ++j) {
            output.push_back(input[pos++] ^ 0x01);
          }
          output.push_back('\n');
        }

        return {std::move(output)};
      }

      auto encode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        std::vector<uint8_t> output;

        if (input.empty()) {
          write_u32_le(output, 0);
          return {std::move(output)};
        }

        // Split on \n, strip trailing empty segment (matches old Qt opener)
        struct Line {
          size_t start;
          size_t len;
        };
        std::vector<Line> lines;
        size_t line_start = 0;
        for (size_t i = 0; i < input.size(); ++i) {
          if (input[i] == '\n') {
            lines.push_back({line_start, i - line_start});
            line_start = i + 1;
          }
        }
        // Last segment (no trailing \n)
        if (line_start < input.size()) {
          lines.push_back({line_start, input.size() - line_start});
        }
        // Strip trailing empty segment
        while (!lines.empty() && lines.back().len == 0) {
          lines.pop_back();
        }

        write_u32_le(output, static_cast<uint32_t>(lines.size()));
        for (auto& line : lines) {
          write_u32_le(output, static_cast<uint32_t>(line.len));
          for (size_t j = line.start; j < line.start + line.len; ++j) {
            output.push_back(input[j] ^ 0x01);
          }
        }

        return {std::move(output)};
      }
    };

    const TextLstCodec kTextLstCodec;

  }  // namespace

  auto text_lst_codec() -> const Codec* { return &kTextLstCodec; }

}  // namespace onex::archive
