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
  using onex::downloader::ProgressCallback;

  class FakeHttpClient : public HttpClient {
  public:
    auto get(const std::string&) -> HttpResponse override { return {}; }
    auto download(const std::string&, const std::string&) -> HttpResponse override { return {}; }
    void set_progress_callback(ProgressCallback) override {}
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

TEST_CASE("download_batch with all folders returns all-skipped") {
  auto d = makeDownloader();
  BuildInfoEntry folder_entry;
  folder_entry.folder = true;
  folder_entry.file = "NostaleData\\SubFolder";

  auto results = d.download_batch({folder_entry, folder_entry}, "/tmp", 2);
  CHECK(results.size() == 2);
  CHECK(results[0].index == 0);
  CHECK(results[1].index == 1);
  CHECK(results[0].status);
  CHECK(results[0].status.value == onex::downloader::FileStatus::kSkipped);
}

TEST_CASE("download_batch uses custom client factory") {
  auto d = makeDownloader();
  BuildInfoEntry e;
  e.file = "NostaleData\\test.txt";
  e.path = "/game/nostale/test.txt";
  e.folder = false;

  bool factory_called = false;
  d.set_client_factory([&]() -> std::unique_ptr<HttpClient> {
    factory_called = true;
    return std::make_unique<FakeHttpClient>();
  });

  auto results = d.download_batch({e}, "/tmp", 1);
  CHECK(factory_called);
  CHECK(results.size() == 1);
}
