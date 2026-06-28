#include <doctest/doctest.h>
#include <onex/archive/nos_archive.h>

#include <filesystem>

#include "fixture.h"

TEST_CASE("NosArchive::open returns kFileNotFound for missing file") {
  auto result = onex::archive::NosArchive::open("/nonexistent/path.NOS");
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kFileNotFound);
}

TEST_CASE("NosArchive::open reads 16-byte header from a real NOS file") {
  auto path = ensure_fixture("NostaleData\\NSetcData.NOS");
  REQUIRE(std::filesystem::exists(path));

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  CHECK(result.value.is_open());
  CHECK(result.value.filepath() == path.string());
  CHECK(result.value.header().size() == 16);
}

TEST_CASE("NosArchive::open header contains known bytes for NSetcData") {
  auto path = ensure_fixture("NostaleData\\NSetcData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  CHECK(h[0] == 0x02);
  CHECK(h[12] == 'M');
  CHECK(h[13] == 'i');
  CHECK(h[14] == 'n');
  CHECK(h[15] == 'i');
}
