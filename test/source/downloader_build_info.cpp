#include <doctest/doctest.h>
#include <onex/core/error.h>
#include <onex/downloader/build_info.h>

#include <string>
#include <vector>

TEST_CASE("parse_build_info parses a valid manifest") {
  std::string json_str
      = R"({"totalSize":1234,"build":42,"entries":[)"
        R"({"path":"/foo/NSipData.NOS","sha1":"abc","file":"NostaleData\\NSipData.NOS","flags":0,"size":100,"folder":false},)"
        R"({"path":"/foo/","sha1":"","file":"","flags":0,"size":0,"folder":true}]})";
  std::vector<onex::byte> raw(json_str.begin(), json_str.end());

  auto result = onex::downloader::parse_build_info(raw);
  REQUIRE(result);
  CHECK(result.value.build == 42);
  CHECK(result.value.total_size == 1234);
  REQUIRE(result.value.entries.size() == 2);
  CHECK(result.value.entries[0].file == "NostaleData\\NSipData.NOS");
  CHECK(result.value.entries[0].path == "/foo/NSipData.NOS");
  CHECK(result.value.entries[0].sha1 == "abc");
  CHECK(result.value.entries[0].size == 100);
  CHECK(result.value.entries[0].folder == false);
  CHECK(result.value.entries[1].folder == true);
}

TEST_CASE("parse_build_info rejects invalid JSON") {
  std::string bad = "not json";
  std::vector<onex::byte> raw(bad.begin(), bad.end());

  auto result = onex::downloader::parse_build_info(raw);
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kInvalidHeader);
}

