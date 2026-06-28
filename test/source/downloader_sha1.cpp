#include <doctest/doctest.h>
#include <onex/downloader/sha1.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

TEST_CASE("sha1_hex of known inputs") {
  // echo -n "abc" | sha1sum
  std::string s = "abc";
  std::vector<onex::byte> data(s.begin(), s.end());
  CHECK(onex::downloader::sha1_hex(data) == "a9993e364706816aba3e25717850c26c9cd0d89d");

  // echo -n "xyz" | sha1sum
  std::string s2 = "xyz";
  std::vector<onex::byte> data2(s2.begin(), s2.end());
  CHECK(onex::downloader::sha1_hex(data2) == "66b27417d37e024c46526c2f6d358a754fc552f3");
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
