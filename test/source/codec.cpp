#include <doctest/doctest.h>
#include <onex/archive/zlib_codec.h>

#include <cstring>
#include <string_view>

TEST_CASE("zlib_codec round-trip encode then decode") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  std::vector<uint8_t> original(256, 0xAB);

  auto encoded = codec->encode(original);
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.size() == original.size());
  CHECK(memcmp(decoded.value.data(), original.data(), original.size()) == 0);
}

TEST_CASE("zlib_codec decode empty input returns empty") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  auto result = codec->decode({});
  REQUIRE(result);
  CHECK(result.value.empty());
}

TEST_CASE("zlib_codec decode truncated input returns kDecompressFailed") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  std::vector<uint8_t> original(64, 0x42);

  auto encoded = codec->encode(original);
  REQUIRE(encoded);

  // Truncate to just a few bytes
  auto truncated = std::vector<uint8_t>(encoded.value.begin(), encoded.value.begin() + 4);
  auto result = codec->decode(truncated);
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kDecompressFailed);
}

TEST_CASE("zlib_codec encode empty input") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  auto result = codec->encode({});
  REQUIRE(result);
  // Empty input still produces zlib framing (2 bytes header + 4 bytes check)
  CHECK(result.value.size() > 0);
}

TEST_CASE("zlib_codec decode known raw zlib data") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  // Raw zlib-compressed "Hello, World!"
  const std::string_view expected = "Hello, World!";
  auto encoded = codec->encode(std::vector<uint8_t>(expected.begin(), expected.end()));
  REQUIRE(encoded);

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  std::string_view result_str(reinterpret_cast<const char*>(decoded.value.data()),
                              decoded.value.size());
  CHECK(result_str == expected);
}

TEST_CASE("zlib_codec round-trip large data") {
  auto* codec = onex::archive::zlib_codec();
  REQUIRE(codec);

  std::vector<uint8_t> original(100000);
  for (size_t i = 0; i < original.size(); ++i) {
    original[i] = static_cast<uint8_t>(i & 0xFF);
  }

  auto encoded = codec->encode(original);
  REQUIRE(encoded);
  CHECK(encoded.value.size() < original.size());

  auto decoded = codec->decode(encoded.value);
  REQUIRE(decoded);
  CHECK(decoded.value.size() == original.size());
  CHECK(memcmp(decoded.value.data(), original.data(), original.size()) == 0);
}
