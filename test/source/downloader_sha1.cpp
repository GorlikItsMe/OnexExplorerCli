#include <doctest/doctest.h>
#include <onex/downloader/sha1.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

TEST_CASE("sha1_hex of known inputs") {
  // SHA-1("abc") per RFC 3174 / FIPS 180-1:
  //   A9 99 3E 36 47 06 81 6A BA 3E 25 71 78 50 C2 6C 9C D0 D8 9D
  std::string s = "abc";
  std::vector<onex::byte> data(s.begin(), s.end());
  CHECK(onex::downloader::sha1_hex(data) == "a9993e364706816aba3e25717850c26c9cd0d89d");

  // SHA-1("") = da39a3ee5e6b4b0d3255bfef95601890afd80709
  std::vector<onex::byte> empty;
  CHECK(onex::downloader::sha1_hex(empty) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST_CASE("sha1_file_hex matches sha1_hex for the same content") {
  std::string payload = "The quick brown fox jumps over the lazy dog";
  auto path = (std::filesystem::temp_directory_path() / "onex_sha1_test.bin").string();

  {
    std::ofstream out(path, std::ios::binary);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  }

  std::vector<onex::byte> data(payload.begin(), payload.end());
  CHECK(onex::downloader::sha1_file_hex(path) == onex::downloader::sha1_hex(data));

  std::filesystem::remove(path);
}

TEST_CASE("sha1_file_hex returns empty string for missing file") {
  CHECK(onex::downloader::sha1_file_hex("/nonexistent/onex_sha1_missing.bin").empty());
}
