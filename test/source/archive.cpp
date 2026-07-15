#include <doctest/doctest.h>
#include <onex/archive/archive_format.h>
#include <onex/archive/nos_archive.h>
#include <onex/archive/zlib_archive_format.h>
#include <onex/archive/zlib_codec.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include "fixture.h"

// ---------------------------------------------------------------------------
// NosArchive::open – basic error cases
// ---------------------------------------------------------------------------

TEST_CASE("NosArchive::open returns kFileNotFound for missing file") {
  auto result = onex::archive::NosArchive::open("/nonexistent/path.NOS");
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kFileNotFound);
}

TEST_CASE("NosArchive::open returns kInvalidFormat for random garbage file") {
  auto temp_dir = std::filesystem::path(ONEX_PROJECT_SOURCE_DIR) / "temp" / "test_invalid";
  std::filesystem::create_directories(temp_dir);
  auto file_path = temp_dir / "garbage.nos";

  // Write 256 bytes of binary garbage
  {
    std::ofstream ofs(file_path, std::ios::binary);
    for (int i = 0; i < 256; ++i) {
      ofs.put(static_cast<char>(i));
    }
  }

  auto result = onex::archive::NosArchive::open(file_path.string());
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kInvalidFormat);

  std::filesystem::remove(file_path);
  std::filesystem::remove(temp_dir);
}

TEST_CASE("NosArchive::open reads 16-byte header from a real NOS file") {
  // NSetcData.NOS — 1 KB, text archive
  auto path = ensure_fixture("NostaleData\\NSetcData.NOS");
  REQUIRE(std::filesystem::exists(path));

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  CHECK(result.value.is_open());
  CHECK(result.value.filepath() == path.string());
  CHECK(result.value.header().size() == 16);
}

// ---------------------------------------------------------------------------
// NosArchive::open – zlib-format archives
// ---------------------------------------------------------------------------

TEST_CASE("NosArchive::open parses NSipData.NOS (NT Data 24)") {
  // NSipData.NOS — 9.7 MB, zlib format
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected
      = {'N', 'T', ' ', 'D', 'a', 't', 'a', ' ', '2', '4', 0x00, 0x00};
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));

  CHECK(result.value.entries().size() > 0);
}

TEST_CASE("NosArchive::open parses NS4BbData.NOS (32GBS V1.0)") {
  // NS4BbData.NOS — 79.4 MB, zlib format
  auto path = ensure_fixture("NostaleData\\NS4BbData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected = {
      '3', '2', 'G', 'B', 'S', ' ', 'V', '1', '.', '0',
  };
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));

  CHECK(result.value.entries().size() > 0);
}

TEST_CASE("NosArchive::open header does not contain known bytes for NSgtdData.NOS") {
  // NSgtdData.NOS — 17.3 MB, text archive
  auto path = ensure_fixture("NostaleData\\NSgtdData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> not_expected = {'N', 'T', ' ', 'D', 'a', 't', 'a'};
  CHECK(std::equal(not_expected.begin(), not_expected.end(), h.begin()) == false);
}

TEST_CASE("NosArchive::open header contains known bytes for NSmnData.NOS" * doctest::skip(true)) {
  // Skipped until a CCINF-archive format parser is implemented
  // NSmnData.NOS — 0.5 MB, CCINF V1.20 — currently returns kInvalidFormat
  auto path = ensure_fixture("NostaleData\\NSmnData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& h = result.value.header();
  const std::vector<uint8_t> expected = {
      'C', 'C', 'I', 'N', 'F', ' ', 'V', '1', '.', '2', '0',
  };
  CHECK(std::equal(expected.begin(), expected.end(), h.begin()));
}

// ---------------------------------------------------------------------------
// ArchiveFormat::detect – header magic detection
// ---------------------------------------------------------------------------

TEST_CASE("ArchiveFormat::detect returns ZlibArchiveFormat for NT Data") {
  std::vector<uint8_t> header{'N', 'T', ' ',  'D',  'a',  't',  'a',  ' ',
                              '2', '4', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  REQUIRE(fmt);
}

TEST_CASE("ArchiveFormat::detect returns ZlibArchiveFormat for 32GBS V1.0") {
  std::vector<uint8_t> header{'3', '2', 'G',  'B',  'S',  ' ',  'V',  '1',
                              '.', '0', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  REQUIRE(fmt);
}

TEST_CASE("ArchiveFormat::detect returns ZlibArchiveFormat for ITEMS V1.0") {
  std::vector<uint8_t> header{'I', 'T', 'E',  'M',  'S',  ' ',  'V',  '1',
                              '.', '0', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  REQUIRE(fmt);
}

TEST_CASE("ArchiveFormat::detect returns TextArchiveFormat for CCINF" * doctest::skip(true)) {
  // Skipped until a CCINF-archive format parser is implemented
  // CCINF is not a text archive — TextArchiveFormat is a wrong fallback
  std::vector<uint8_t> header{'C', 'C', 'I', 'N',  'F',  ' ',  'V',  '1',
                              '.', '2', '0', 0x00, 0x00, 0x00, 0x00, 0x00};
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  REQUIRE(fmt);
}

TEST_CASE("ArchiveFormat::detect returns nullptr for invalid header") {
  std::vector<uint8_t> header(3, 0x00);
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  CHECK_FALSE(fmt);
}

// ---------------------------------------------------------------------------
// ZlibArchiveFormat::parse_entry_table – real file integration
// ---------------------------------------------------------------------------

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns entries from NStpData01.NOS") {
  auto path = ensure_fixture("NostaleData\\NStpData01.NOS");

  std::ifstream file(path, std::ios::binary);
  REQUIRE(file.is_open());

  std::array<uint8_t, 16> header{};
  file.read(reinterpret_cast<char*>(header.data()), 16);
  REQUIRE(file);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(header, file);
  REQUIRE(result);
  CHECK(result.value.size() > 0);

  // All entries in NStpData01 are Texture type
  for (const auto& e : result.value) {
    CHECK(e.type == onex::archive::EntryType::Texture);
  }
}

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns entries from NSipData.NOS") {
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");

  std::ifstream file(path, std::ios::binary);
  REQUIRE(file.is_open());

  std::array<uint8_t, 16> header{};
  file.read(reinterpret_cast<char*>(header.data()), 16);
  REQUIRE(file);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(header, file);
  REQUIRE(result);
  CHECK(result.value.size() > 0);

  // All entries in NSipData are Icon type
  for (const auto& e : result.value) {
    CHECK(e.type == onex::archive::EntryType::Icon);
  }
}

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns entries from NS4BbData.NOS") {
  auto path = ensure_fixture("NostaleData\\NS4BbData.NOS");

  std::ifstream file(path, std::ios::binary);
  REQUIRE(file.is_open());

  std::array<uint8_t, 16> header{};
  file.read(reinterpret_cast<char*>(header.data()), 16);
  REQUIRE(file);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(header, file);
  REQUIRE(result);
  CHECK(result.value.size() > 0);

  // All entries in NS4BbData are Image4B type
  for (const auto& e : result.value) {
    CHECK(e.type == onex::archive::EntryType::Image4B);
  }
}

TEST_CASE(
    "ZlibArchiveFormat::parse_entry_table returns kInvalidFormat for truncated offset table") {
  // Buffer has header + fileCount=100 but only 2 entries of offset data
  std::vector<uint8_t> buf{
      'N',  'T',  ' ',  'D',  'a',  't',  'a',  ' ',
      '0',  '7',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // header
      100,  0x00, 0x00, 0x00,                          // fileCount = 100
      0x00,                                            // separator
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // only 1 entry in table
  };

  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);
  stream.ignore(16);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(buf, stream);
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kInvalidFormat);
}

// ---------------------------------------------------------------------------
// Text archive integration – real file decode
// ---------------------------------------------------------------------------

TEST_CASE("NosArchive::read_entry on NScliData_UK.NOS first entry contains 'Info'") {
  auto path = ensure_fixture("NostaleData\\NScliData_UK.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto entries = result.value.entries();
  REQUIRE(entries.size() > 0);
  CHECK(entries[0].type == onex::archive::EntryType::TextDat);
  CHECK(entries[0].name == "conststring.dat");

  auto data = result.value.read_entry(0);
  REQUIRE(data);

  // The decrypted content should contain readable game strings
  std::string_view content(reinterpret_cast<const char*>(data.value.data()), data.value.size());
  CHECK(content.find("Info") != std::string_view::npos);
  CHECK(content.find("OK") != std::string_view::npos);
  CHECK(content.find("Name") != std::string_view::npos);
}

// ---------------------------------------------------------------------------
// NosArchive::read_entry – integration with real fixture
// ---------------------------------------------------------------------------

TEST_CASE("NosArchive::read_entry returns decompressed data for first entry") {
  auto path = ensure_fixture("NostaleData\\NStpData01.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& archive = result.value;
  REQUIRE(archive.entries().size() > 0);

  auto entry = archive.entries()[0];
  auto data = archive.read_entry(0);
  REQUIRE(data);
  CHECK(data.value.size() == entry.uncompressed_size);
}

TEST_CASE("NosArchive::read_entry returns kEntryNotFound for bad index") {
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto data = result.value.read_entry(999999);
  CHECK_FALSE(data);
  CHECK(data.error == onex::Error::kEntryNotFound);
}

TEST_CASE("NosArchive::read_entry each entry has matching uncompressed_size") {
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");

  auto result = onex::archive::NosArchive::open(path);
  REQUIRE(result);

  auto& archive = result.value;
  // Test first 5 entries
  auto count = std::min<size_t>(5, archive.entries().size());
  for (size_t i = 0; i < count; ++i) {
    auto data = archive.read_entry(i);
    REQUIRE(data);
    CHECK(data.value.size() == archive.entries()[i].uncompressed_size);
  }
}

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns kReadError for offset past EOF") {
  std::vector<uint8_t> buf{
      'N',  'T',  ' ',  'D',  'a',  't',  'a',  ' ',
      '0',  '7',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // header
      0x01, 0x00, 0x00, 0x00,                          // fileCount = 1
      0x00,                                            // separator
      0x00, 0x00, 0x00, 0x00,                          // id = 0
      0xFF, 0xFF, 0xFF, 0x7F,                          // offset = 0x7FFFFFFF (way past EOF)
  };

  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);
  stream.ignore(16);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(buf, stream);
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kReadError);
}

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns kReadError for truncated entry header") {
  // Offset table points to a valid offset, but there's not enough data there
  std::vector<uint8_t> buf{
      'N',  'T',  ' ',  'D',  'a',  't',  'a',  ' ',
      '0',  '7',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // header
      0x01, 0x00, 0x00, 0x00,                          // fileCount = 1
      0x00,                                            // separator
      0x00, 0x00, 0x00, 0x00,                          // id = 0
      0x21, 0x00, 0x00, 0x00,                          // offset = 33
  };
  // Only 2 bytes at offset 33 — not enough for entry header (13 bytes)
  buf.resize(35, 0x00);

  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);
  stream.ignore(16);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(buf, stream);
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kReadError);
}
