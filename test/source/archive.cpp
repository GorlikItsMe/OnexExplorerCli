#include <doctest/doctest.h>
#include <onex/archive/archive_format.h>
#include <onex/archive/nos_archive.h>
#include <onex/archive/zlib_archive_format.h>

#include <filesystem>
#include <sstream>

#include "fixture.h"

// ---------------------------------------------------------------------------
// NosArchive::open – basic error cases
// ---------------------------------------------------------------------------

TEST_CASE("NosArchive::open returns kFileNotFound for missing file") {
  auto result = onex::archive::NosArchive::open("/nonexistent/path.NOS");
  CHECK_FALSE(result);
  CHECK(result.error == onex::Error::kFileNotFound);
}

TEST_CASE("NosArchive::open reads 16-byte header from a real NOS file" * doctest::skip(true)) {
  // Skipped until a text-archive format parser is implemented (issue #2 handles zlib only)
  // NSetcData.NOS — 1 KB, text archive — currently returns kInvalidFormat
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

TEST_CASE("NosArchive::open header does not contain known bytes for NSgtdData.NOS"
          * doctest::skip(true)) {
  // Skipped until a text-archive format parser is implemented
  // NSgtdData.NOS — 17.3 MB, text archive — currently returns kInvalidFormat
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

TEST_CASE("ArchiveFormat::detect returns nullptr for unknown magic") {
  std::vector<uint8_t> header{'C', 'C', 'I', 'N',  'F',  ' ',  'V',  '1',
                              '.', '2', '0', 0x00, 0x00, 0x00, 0x00, 0x00};
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  CHECK_FALSE(fmt);
}

TEST_CASE("ArchiveFormat::detect returns nullptr for short header") {
  std::vector<uint8_t> header(3, 0x00);
  auto fmt = onex::archive::ArchiveFormat::detect(header);
  CHECK_FALSE(fmt);
}

// ---------------------------------------------------------------------------
// ZlibArchiveFormat::parse_entry_table – synthetic buffer
// ---------------------------------------------------------------------------

namespace {

  auto make_synthetic_nt_data() -> std::vector<uint8_t> {
    // 16-byte header: "NT Data 07" + padding
    std::vector<uint8_t> buf{
        'N', 'T', ' ', 'D', 'a', 't', 'a', ' ', '0', '7', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    // fileCount = 2 (LE)
    buf.push_back(0x02);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    // separator byte
    buf.push_back(0x00);

    // --- offset table (2 entries) ---
    // Entry 0: id=0, offset = 16+5+16 = 37 = 0x25
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);  // id 0
    buf.push_back(0x25);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);  // offset 0x25
    // Entry 1: id=1, offset = 0x25 + 13 = 0x32
    buf.push_back(0x01);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);  // id 1
    buf.push_back(0x32);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);  // offset 0x32

    // --- Entry 0 at offset 0x25 ---
    // creationDate = 0x01357A4F (20230615)
    buf.push_back(0x4F);
    buf.push_back(0x7A);
    buf.push_back(0x35);
    buf.push_back(0x01);
    // dataSize = 100
    buf.push_back(0x64);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    // compressedDataSize = 80
    buf.push_back(0x50);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    // isCompressed = 1
    buf.push_back(0x01);

    // --- Entry 1 at offset 0x32 ---
    // creationDate = 0x01359190 (20230616)
    buf.push_back(0x90);
    buf.push_back(0x91);
    buf.push_back(0x35);
    buf.push_back(0x01);
    // dataSize = 200
    buf.push_back(0xC8);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    // compressedDataSize = 200
    buf.push_back(0xC8);
    buf.push_back(0x00);
    buf.push_back(0x00);
    buf.push_back(0x00);
    // isCompressed = 0
    buf.push_back(0x00);

    return buf;
  }

}  // namespace

TEST_CASE("ZlibArchiveFormat::parse_entry_table returns entries from synthetic buffer") {
  auto buf = make_synthetic_nt_data();
  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);
  stream.ignore(16);  // skip header — stream must be positioned after it

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(buf, stream);
  REQUIRE(result);
  CHECK(result.value.size() == 2);

  // Entry 0
  CHECK(result.value[0].id == 0);
  CHECK(result.value[0].name == "0");
  CHECK(result.value[0].creation_date == 0x01357A4F);
  CHECK(result.value[0].compressed == true);
  CHECK(result.value[0].type == onex::archive::EntryType::Texture);
  CHECK(result.value[0].offset == 0x25);
  CHECK(result.value[0].uncompressed_size == 100);
  CHECK(result.value[0].compressed_size == 80);

  // Entry 1
  CHECK(result.value[1].id == 1);
  CHECK(result.value[1].name == "1");
  CHECK(result.value[1].creation_date == 0x01359190);
  CHECK(result.value[1].compressed == false);
  CHECK(result.value[1].type == onex::archive::EntryType::Texture);
  CHECK(result.value[1].offset == 0x32);
  CHECK(result.value[1].uncompressed_size == 200);
  CHECK(result.value[1].compressed_size == 200);
}

TEST_CASE("ZlibArchiveFormat::parse_entry_table handles duplicate names") {
  auto buf = make_synthetic_nt_data();
  // Override id of entry 1 to 0 (duplicate)
  buf[0x1D] = 0x00;

  std::string data(reinterpret_cast<const char*>(buf.data()), buf.size());
  std::istringstream stream(data);
  stream.ignore(16);

  onex::archive::ZlibArchiveFormat fmt;
  auto result = fmt.parse_entry_table(buf, stream);
  REQUIRE(result);
  CHECK(result.value.size() == 2);
  CHECK(result.value[0].name == "0");
  CHECK(result.value[1].name == "0_2");
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
