#include <doctest/doctest.h>
#include <zlib.h>

#include <cstring>
#include <vector>

TEST_CASE("zlib compress/uncompress roundtrip") {
  // Generate predictable pseudo-random data
  std::vector<unsigned char> original(65536);
  for (size_t i = 0; i < original.size(); ++i) {
    original[i] = static_cast<unsigned char>((i * 1103515245 + 12345) & 0xFF);
  }

  // Compress
  uLongf compressed_size = compressBound(original.size());
  std::vector<unsigned char> compressed(compressed_size);
  int ret = compress(compressed.data(), &compressed_size, original.data(), original.size());
  REQUIRE(ret == Z_OK);

  // Decompress
  uLongf decompressed_size = original.size();
  std::vector<unsigned char> decompressed(decompressed_size);
  ret = uncompress(decompressed.data(), &decompressed_size, compressed.data(), compressed_size);
  REQUIRE(ret == Z_OK);
  REQUIRE(decompressed_size == original.size());

  // Compare
  CHECK(std::memcmp(original.data(), decompressed.data(), original.size()) == 0);
}
