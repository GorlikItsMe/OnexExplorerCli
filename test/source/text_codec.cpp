#include <doctest/doctest.h>
#include <onex/archive/text_archive_format.h>
#include <onex/archive/text_dat_codec.h>
#include <onex/archive/text_lst_codec.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string_view>
#include <vector>

// ---------------------------------------------------------------------------
// text_dat_codec tests
//
// Round-trip behavior: decode(encode(input)) == input + trailing "\r"
// The final 0xFF segment terminator always becomes 0x0D on decode,
// matching the original Qt NosTextDatFileDecryptor behavior.
// When input already ends with \r, the trailing empty segment is stripped
// during encode, so the round-trip is exact.
// ---------------------------------------------------------------------------

TEST_CASE("text_dat_codec round-trip single segment") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // Single segment "Hello" → decode(encode) = "Hello\r" (trailing \r added)
  std::vector<uint8_t> input = {'H', 'e', 'l', 'l', 'o'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == input.size() + 1);
  CHECK(decoded.value.back() == 0x0D);
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_dat_codec round-trip with carriage returns") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // "Hello\rWorld" → decode(encode) = "Hello\rWorld\r" (trailing \r added)
  std::vector<uint8_t> input = {'H', 'e', 'l', 'l', 'o', 0x0D, 'W', 'o', 'r', 'l', 'd'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == input.size() + 1);
  CHECK(decoded.value.back() == 0x0D);
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_dat_codec round-trip multiple segments with trailing CR") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // "a\rb\rc\r" → input already ends with \r, trailing empty stripped
  // decode(encode) = "a\rb\rc\r" (exact match)
  std::vector<uint8_t> input = {'a', 0x0D, 'b', 0x0D, 'c', 0x0D};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.size() == input.size());
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_dat_codec round-trip empty input") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // Empty → encode produces [0xFF], decode produces [0x0D]
  std::vector<uint8_t> input{};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);
  CHECK(encoded.value.size() == 1);
  CHECK(encoded.value[0] == 0xFF);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == 1);
  CHECK(decoded.value[0] == 0x0D);
}

TEST_CASE("text_dat_codec round-trip long segment") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // Long enough to trigger 0x7E chunk boundaries
  std::vector<uint8_t> input(300, 'x');
  input[100] = 0x0D;
  input[200] = 0x0D;

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == input.size() + 1);
  CHECK(decoded.value.back() == 0x0D);
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_dat_codec known-answer test") {
  auto* codec = onex::archive::text_dat_codec();
  REQUIRE(codec);

  // 'a' = 0x61 — NOT printable (mask=0x30), goes through literal XOR path
  // Run: prefix 1, byte a^0x33 = 0x52, then 0xFF terminator
  std::vector<uint8_t> input = {'a'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  REQUIRE(encoded.value.size() >= 3);
  CHECK(encoded.value[0] == 1);     // literal run length = 1
  CHECK(encoded.value[1] == 0x52);  // 'a' ^ 0x33
  CHECK(encoded.value.back() == 0xFF);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == 2);
  CHECK(decoded.value[0] == 'a');
  CHECK(decoded.value[1] == 0x0D);
}

// ---------------------------------------------------------------------------
// text_lst_codec tests
//
// Round-trip behavior: decode(encode(input)) == input with exact line breaks.
// Lines are split on \n, trailing empty segment is stripped.
// When input ends with \n, round-trip is exact.
// When input does not end with \n, decode adds one.
// ---------------------------------------------------------------------------

TEST_CASE("text_lst_codec round-trip single line with trailing newline") {
  auto* codec = onex::archive::text_lst_codec();
  REQUIRE(codec);

  std::vector<uint8_t> input = {'H', 'e', 'l', 'l', 'o', '\n'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.size() == input.size());
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_lst_codec round-trip single line without trailing newline") {
  auto* codec = onex::archive::text_lst_codec();
  REQUIRE(codec);

  // "Hello" → decode(encode) = "Hello\n" (trailing \n added)
  std::vector<uint8_t> input = {'H', 'e', 'l', 'l', 'o'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  REQUIRE(decoded.value.size() == input.size() + 1);
  CHECK(decoded.value.back() == '\n');
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_lst_codec round-trip multiple lines with trailing newline") {
  auto* codec = onex::archive::text_lst_codec();
  REQUIRE(codec);

  std::vector<uint8_t> input = {'L', 'i', 'n',  'e', '1', '\n', 'L', 'i', 'n',
                                'e', '2', '\n', 'L', 'i', 'n',  'e', '3', '\n'};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.size() == input.size());
  CHECK(memcmp(decoded.value.data(), input.data(), input.size()) == 0);
}

TEST_CASE("text_lst_codec round-trip empty input") {
  auto* codec = onex::archive::text_lst_codec();
  REQUIRE(codec);

  std::vector<uint8_t> input{};

  auto encoded = codec->encode(input);
  REQUIRE(encoded);
  CHECK(encoded.value.size() == 4);  // line count = 0

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.empty());
}

TEST_CASE("text_lst_codec decode corrupt data returns kCorruptArchive") {
  auto* codec = onex::archive::text_lst_codec();
  REQUIRE(codec);

  std::vector<uint8_t> encrypted
      = {0x01, 0x00, 0x00, 0x00,   // line count = 1
         0x10, 0x00, 0x00, 0x00};  // line length = 16, but only 8 bytes total

  auto decoded = codec->decode(encrypted);
  CHECK_FALSE(decoded);
  CHECK(decoded.error == onex::Error::kCorruptArchive);
}

// ---------------------------------------------------------------------------
// TextArchiveFormat tests
// ---------------------------------------------------------------------------

TEST_CASE("TextArchiveFormat parse_entry_table returns kInvalidFormat for truncated data") {
  std::vector<uint8_t> buf(4, 0x01);
  buf.resize(16, 0);

  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);

  std::array<uint8_t, 16> header{};
  stream.read(reinterpret_cast<char*>(header.data()), 16);

  onex::archive::TextArchiveFormat fmt;
  auto result = fmt.parse_entry_table(header, stream);
  CHECK_FALSE(result);
}
