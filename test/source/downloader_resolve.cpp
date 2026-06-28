#include <doctest/doctest.h>
#include <onex/core/error.h>
#include <onex/downloader/downloader.h>
#include <onex/downloader/http_client.h>

#include <memory>
#include <string>
#include <vector>

namespace {

  using onex::downloader::BuildInfoEntry;
  using onex::downloader::GameforgeDownloader;
  using onex::downloader::HttpClient;
  using onex::downloader::HttpResponse;

  class FakeHttpClient : public HttpClient {
  public:
    auto get(const std::string&) -> HttpResponse override { return {}; }
    auto download(const std::string&, const std::string&) -> HttpResponse override { return {}; }
  };

  auto makeEntry(std::string file, std::int64_t size = 100) -> BuildInfoEntry {
    BuildInfoEntry e;
    e.file = std::move(file);
    e.size = size;
    e.folder = false;
    return e;
  }

  auto makeDownloader() -> GameforgeDownloader {
    return GameforgeDownloader{"nostale", "latest", std::make_unique<FakeHttpClient>()};
  }

}  // namespace

TEST_CASE("resolve matches exact file field") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NostaleData\\NSipData.NOS"),
      makeEntry("NostaleData\\NSgtdData.NOS"),
  };

  auto r = d.resolve(entries, "NostaleData\\NSipData.NOS");
  REQUIRE(r);
  CHECK(r.value.file == "NostaleData\\NSipData.NOS");
}

TEST_CASE("resolve falls back to bare filename match") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NostaleData\\NSipData.NOS"),
      makeEntry("NostaleData\\NSgtdData.NOS"),
  };

  auto r = d.resolve(entries, "NSipData.NOS");
  REQUIRE(r);
  CHECK(r.value.file == "NostaleData\\NSipData.NOS");
}

TEST_CASE("resolve prefers exact match over bare match") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NSipData.NOS"),
      makeEntry("NostaleData\\NSipData.NOS"),
  };

  auto r = d.resolve(entries, "NSipData.NOS");
  REQUIRE(r);
  CHECK(r.value.file == "NSipData.NOS");
}

TEST_CASE("resolve reports ambiguous bare filename match") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NostaleData\\NSipData.NOS"),
      makeEntry("Other\\NSipData.NOS"),
  };

  auto r = d.resolve(entries, "NSipData.NOS");
  CHECK_FALSE(r);
  CHECK(r.error == onex::Error::kAmbiguousMatch);
}

TEST_CASE("resolve reports ambiguous exact match") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NSipData.NOS", 100),
      makeEntry("NSipData.NOS", 200),
  };

  auto r = d.resolve(entries, "NSipData.NOS");
  CHECK_FALSE(r);
  CHECK(r.error == onex::Error::kAmbiguousMatch);
}

TEST_CASE("resolve reports not found when nothing matches") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NostaleData\\NSipData.NOS"),
  };

  auto r = d.resolve(entries, "DoesNotExist.NOS");
  CHECK_FALSE(r);
  CHECK(r.error == onex::Error::kEntryNotFound);
}

TEST_CASE("resolve ignores folder entries") {
  auto d = makeDownloader();
  BuildInfoEntry folder;
  folder.file = "NSipData.NOS";
  folder.folder = true;
  std::vector<BuildInfoEntry> entries = {folder};

  auto r = d.resolve(entries, "NSipData.NOS");
  CHECK_FALSE(r);
  CHECK(r.error == onex::Error::kEntryNotFound);
}

TEST_CASE("resolve does not auto-append .NOS") {
  auto d = makeDownloader();
  std::vector<BuildInfoEntry> entries = {
      makeEntry("NostaleData\\NSipData.NOS"),
  };

  auto r = d.resolve(entries, "NSipData");
  CHECK_FALSE(r);
  CHECK(r.error == onex::Error::kEntryNotFound);
}
