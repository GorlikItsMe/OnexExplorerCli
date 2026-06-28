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

TEST_CASE("NosArchive::open header contains known bytes for NSipData.NOS") {
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected
      = {'N', 'T', ' ', 'D', 'a', 't', 'a', ' ', '2', '4', 0x00, 0x00};
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));
}

TEST_CASE("NosArchive::open header does not contain known bytes for NSgtdData.NOS") {
  auto path = ensure_fixture("NostaleData\\NSgtdData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> not_expected = {'N', 'T', ' ', 'D', 'a', 't', 'a'};
  CHECK(std::equal(not_expected.begin(), not_expected.end(), h.begin()) == false);
}

TEST_CASE("NosArchive::open header contains known bytes for NSmnData.NOS") {
  auto path = ensure_fixture("NostaleData\\NSmnData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected = {
      'C', 'C', 'I', 'N', 'F', ' ', 'V', '1', '.', '2', '0',
  };
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));
}

TEST_CASE("NosArchive::open header contains known bytes for NS4BbData.NOS") {
  auto path = ensure_fixture("NostaleData\\NS4BbData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected = {
      '3', '2', 'G', 'B', 'S', ' ', 'V', '1', '.', '0',
  };
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));
}
