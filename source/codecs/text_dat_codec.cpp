#include <onex/archive/text_dat_codec.h>

#include <array>
#include <cstdint>
#include <vector>

namespace onex::archive {

  namespace {

    constexpr std::array<uint8_t, 16> kCryptoArray
        = {0x00, 0x20, 0x2D, 0x2E, 0x30, 0x31, 0x32, 0x33,
           0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x0A, 0x00};

    auto is_printable(uint8_t ch) -> bool {
      if (ch == 0) {
        return false;
      }
      ch -= 0x20;
      if (ch == 0) {
        return true;
      }
      ch -= 0x0D;
      if (ch < 2) {
        return true;
      }
      ch -= 0x03;
      bool test1 = ch < 0x0A;
      ch -= 0x0A;
      if (!test1 && ch != 0xC5) {
        return false;
      }
      return true;
    }

    auto generate_mask(std::span<const uint8_t> input) -> std::vector<uint8_t> {
      std::vector<uint8_t> mask(input.size());
      for (size_t i = 0; i < input.size(); ++i) {
        mask[i] = is_printable(input[i]) ? 0x31 : 0x30;
      }
      return mask;
    }

    auto char_to_nibble(uint8_t ch) -> uint8_t {
      switch (ch) {
        case 32:
          return 1;
        case 45:
          return 2;
        case 46:
          return 3;
        case 0xFF:
          return 14;
        default:
          return ch - 0x2C;
      }
    }

    struct TextDatCodec final : Codec {
      auto decode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        std::vector<uint8_t> output;
        size_t pos = 0;

        while (pos < input.size()) {
          uint8_t ctrl = input[pos++];

          if (ctrl == 0xFF) {
            output.push_back(0x0D);
            continue;
          }

          uint8_t run_len = ctrl & 0x7F;

          if (ctrl & 0x80) {
            for (uint8_t i = 0; i < run_len; i += 2) {
              if (pos >= input.size()) {
                break;
              }
              uint8_t packed = input[pos++];
              uint8_t hi = static_cast<uint8_t>((packed >> 4) & 0x0F);
              uint8_t lo = static_cast<uint8_t>(packed & 0x0F);
              output.push_back(kCryptoArray[hi]);
              if (i + 1 < run_len) {
                if (kCryptoArray[lo] == 0) {
                  break;
                }
                output.push_back(kCryptoArray[lo]);
              }
            }
          } else {
            for (uint8_t i = 0; i < run_len; ++i) {
              if (pos >= input.size()) {
                break;
              }
              output.push_back(input[pos++] ^ 0x33);
            }
          }
        }

        return {std::move(output)};
      }

      auto encode(std::span<const uint8_t> input) const -> Result<std::vector<uint8_t>> override {
        if (input.empty()) {
          std::vector<uint8_t> segments = {0xFF};
          return {std::move(segments)};
        }

        // Split on \r (0x0D), strip trailing empty segment (matches old Qt code)
        std::vector<std::span<const uint8_t>> raw_segments;
        size_t line_start = 0;
        for (size_t i = 0; i <= input.size(); ++i) {
          if (i == input.size() || input[i] == 0x0D) {
            raw_segments.push_back(input.subspan(line_start, i - line_start));
            line_start = i + 1;
          }
        }
        if (!raw_segments.empty() && raw_segments.back().empty()) {
          raw_segments.pop_back();
        }

        std::vector<uint8_t> output;
        for (auto& line : raw_segments) {
          auto mask = generate_mask(line);
          size_t pos = 0;

          while (pos < line.size()) {
            size_t lit_start = pos;
            while (pos < line.size() && mask[pos] == 0x30) {
              ++pos;
            }
            size_t lit_len = pos - lit_start;
            if (lit_len > 0) {
              for (size_t chunk = 0; chunk < lit_len; chunk += 0x7E) {
                size_t chunk_size = (std::min)(lit_len - chunk, size_t{0x7E});
                output.push_back(static_cast<uint8_t>(chunk_size));
                for (size_t j = 0; j < chunk_size; ++j) {
                  output.push_back(line[lit_start + chunk + j] ^ 0x33);
                }
              }
            }

            if (pos >= line.size()) {
              break;
            }

            size_t char_start = pos;
            while (pos < line.size() && mask[pos] == 0x31) {
              ++pos;
            }
            size_t char_len = pos - char_start;
            if (char_len > 0) {
              for (size_t chunk = 0; chunk < char_len; chunk += 0x7E) {
                size_t chunk_size = (std::min)(char_len - chunk, size_t{0x7E});
                output.push_back(static_cast<uint8_t>(chunk_size) | 0x80);
                bool first_in_pair = true;
                for (size_t j = 0; j < chunk_size; ++j) {
                  uint8_t nibble = char_to_nibble(line[char_start + chunk + j]);
                  if (first_in_pair) {
                    output.push_back(static_cast<uint8_t>(nibble << 4));
                    first_in_pair = false;
                  } else {
                    output.back() |= nibble;
                    first_in_pair = true;
                  }
                }
              }
            }
          }

          output.push_back(0xFF);
        }

        return {std::move(output)};
      }
    };

    const TextDatCodec kTextDatCodec;

  }  // namespace

  auto text_dat_codec() -> const Codec* { return &kTextDatCodec; }

}  // namespace onex::archive
