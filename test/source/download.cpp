#include <curl/curl.h>
#include <doctest/doctest.h>
#include <onex/sha1.h>

#include <onex/download.hpp>
#include <stdexcept>
#include <string>
#include <vector>

struct CurlFixture {
  CurlFixture() { curl_global_init(CURL_GLOBAL_ALL); }
  ~CurlFixture() { curl_global_cleanup(); }
};

TEST_CASE_FIXTURE(CurlFixture, "download - match_archive exact match") {
  auto manifest = onex::fetch_manifest("latest");
  REQUIRE_FALSE(manifest.empty());

  auto* entry = onex::match_archive(manifest, manifest[0].file);
  REQUIRE(entry != nullptr);
  CHECK(entry->file == manifest[0].file);
  CHECK(entry->size > 0);
  CHECK_FALSE(entry->sha1.empty());
}

TEST_CASE_FIXTURE(CurlFixture, "download - match_archive by bare filename") {
  auto manifest = onex::fetch_manifest("latest");
  REQUIRE_FALSE(manifest.empty());

  auto bare = onex::basename(manifest[0].file);
  auto* entry = onex::match_archive(manifest, bare);
  REQUIRE(entry != nullptr);
}

TEST_CASE_FIXTURE(CurlFixture, "download - match_archive nonexistent name throws") {
  auto manifest = onex::fetch_manifest("latest");
  REQUIRE_FALSE(manifest.empty());

  CHECK_THROWS_AS(onex::match_archive(manifest, "__nonexistent_file__"), std::runtime_error);
}

TEST_CASE_FIXTURE(CurlFixture, "download - nonexistent build id throws") {
  CHECK_THROWS_AS(onex::fetch_manifest("999999"), std::runtime_error);
}

TEST_CASE_FIXTURE(CurlFixture, "download - manifest entry fields are valid") {
  auto manifest = onex::fetch_manifest("latest");
  REQUIRE_FALSE(manifest.empty());

  for (const auto& entry : manifest) {
    CHECK_FALSE(entry.file.empty());
    CHECK_FALSE(entry.path.empty());
    CHECK(entry.size > 0);
    CHECK_FALSE(entry.sha1.empty());
  }
}

TEST_CASE("sha1 - known string") {
  onex::Sha1 sha1;
  sha1.update("hello");
  CHECK(sha1.hex_digest() == "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

TEST_CASE("sha1 - empty string") {
  onex::Sha1 sha1;
  CHECK(sha1.hex_digest() == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

TEST_CASE("sha1 - multiple updates") {
  onex::Sha1 sha1;
  sha1.update("hello ");
  sha1.update("world");
  CHECK(sha1.hex_digest() == "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");
}
