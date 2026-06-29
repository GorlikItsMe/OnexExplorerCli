#include <doctest/doctest.h>
#include <lodepng.h>
#include <onex/archive/entry.h>
#include <onex/archive/image/converter.h>
#include <onex/archive/image/entry_image.h>
#include <onex/archive/nos_archive.h>

#include <cstring>
#include <string>
#include <vector>

#include "fixture.h"

using namespace onex::archive;

namespace {

  // A decoded image: dimensions + RGBA pixel data.
  struct Image {
    unsigned width = 0;
    unsigned height = 0;
    std::vector<uint8_t> rgba;
  };

  // Loads a reference PNG from test/data/.
  auto load_reference(const char* filename) -> Image {
    const auto path = std::string(ONEX_PROJECT_SOURCE_DIR) + "/test/data/" + filename;
    Image img;
    REQUIRE(lodepng::decode(img.rgba, img.width, img.height, path) == 0);
    return img;
  }

  // Decodes PNG bytes back to RGBA.
  auto decode_png(const std::vector<uint8_t>& png_bytes) -> Image {
    Image img;
    REQUIRE(lodepng::decode(img.rgba, img.width, img.height, png_bytes) == 0);
    return img;
  }

  auto rgba_eq(std::span<const uint8_t> a, std::span<const uint8_t> b) -> bool {
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size()) == 0;
  }

  auto image_eq(const Image& a, const Image& b) -> bool {
    return a.width == b.width && a.height == b.height && rgba_eq(a.rgba, b.rgba);
  }

  // -- Entry header builders: pack width/height + pixel data into an entry buffer.

  auto make_texture_entry(unsigned w, unsigned h, uint8_t format, std::span<const uint8_t> pixels)
      -> std::vector<uint8_t> {
    std::vector<uint8_t> entry(8, 0);
    entry[0] = static_cast<uint8_t>(w & 0xFF);
    entry[1] = static_cast<uint8_t>((w >> 8) & 0xFF);
    entry[2] = static_cast<uint8_t>(h & 0xFF);
    entry[3] = static_cast<uint8_t>((h >> 8) & 0xFF);
    entry[4] = format;
    entry.insert(entry.end(), pixels.begin(), pixels.end());
    return entry;
  }

  auto make_icon_entry(unsigned w, unsigned h, std::span<const uint8_t> pixels)
      -> std::vector<uint8_t> {
    std::vector<uint8_t> entry(13, 0);
    entry[1] = static_cast<uint8_t>(w & 0xFF);
    entry[2] = static_cast<uint8_t>((w >> 8) & 0xFF);
    entry[3] = static_cast<uint8_t>(h & 0xFF);
    entry[4] = static_cast<uint8_t>((h >> 8) & 0xFF);
    entry.insert(entry.end(), pixels.begin(), pixels.end());
    return entry;
  }

  // TileGrid and Image4B both use a 4-byte (width, height) header.
  auto make_simple_entry(unsigned w, unsigned h, std::span<const uint8_t> pixels)
      -> std::vector<uint8_t> {
    std::vector<uint8_t> entry(4, 0);
    entry[0] = static_cast<uint8_t>(w & 0xFF);
    entry[1] = static_cast<uint8_t>((w >> 8) & 0xFF);
    entry[2] = static_cast<uint8_t>(h & 0xFF);
    entry[3] = static_cast<uint8_t>((h >> 8) & 0xFF);
    entry.insert(entry.end(), pixels.begin(), pixels.end());
    return entry;
  }

  // encode → decode and compare to the original RGBA.
  template <typename EncodeFn, typename DecodeFn>
  void round_trip(const Image& img, EncodeFn encode, DecodeFn decode) {
    auto encoded = encode(img.rgba, static_cast<int>(img.width), static_cast<int>(img.height));
    REQUIRE(!encoded.empty());
    auto decoded = decode(encoded, static_cast<int>(img.width), static_cast<int>(img.height));
    CHECK(rgba_eq(decoded, img.rgba));
  }

  // Asserts that decode_entry_to_png fails with kInvalidFormat.
  void require_rejects(const std::vector<uint8_t>& data, EntryType type) {
    auto png = decode_entry_to_png(data, type);
    CHECK_FALSE(png);
    CHECK(png.error == onex::Error::kInvalidFormat);
  }

}  // namespace

// ===========================================================================
// Converter round-trip with reference images
// ===========================================================================

TEST_CASE("GBAR4444 round-trips a reference image") {
  round_trip(load_reference("test_nibble.png"), encode_rgba_to_gbar4444, decode_gbar4444_to_rgba);
}

TEST_CASE("BGRA8888 round-trips a reference image") {
  round_trip(load_reference("test_nibble.png"), encode_rgba_to_bgra8888, decode_bgra8888_to_rgba);
}

TEST_CASE("BARG4444 round-trips a reference image") {
  round_trip(load_reference("test_nibble.png"), encode_rgba_to_barg4444, decode_barg4444_to_rgba);
}

TEST_CASE("ARGB555 round-trips a reference image") {
  round_trip(load_reference("test_5bit.png"), encode_rgba_to_argb555, decode_argb555_to_rgba);
}

TEST_CASE("Grayscale round-trips a reference image") {
  round_trip(load_reference("test_grayscale.png"), encode_rgba_to_grayscale,
             decode_grayscale_to_rgba);
}

// ===========================================================================
// NSTC -- palette round-trip + fallback
// ===========================================================================

TEST_CASE("NSTC round-trips a palette-only reference image") {
  round_trip(load_reference("test_nstc.png"), encode_rgba_to_nstc, decode_nstc_to_rgba);
}

TEST_CASE("NSTC falls back to 0xFF for undefined colours and indices") {
  // Encode: non-palette colour → 0xFF
  const std::vector<uint8_t> rgba{123, 45, 67, 255};
  auto encoded = encode_rgba_to_nstc(rgba, 1, 1);
  REQUIRE(encoded.size() == 1);
  CHECK(encoded[0] == 0xFF);

  // Decode: undefined index → fallback colour (150, 0, 255)
  const std::vector<uint8_t> index{0x05};
  auto decoded = decode_nstc_to_rgba(index, 1, 1);
  REQUIRE(decoded.size() == 4);
  CHECK(decoded[0] == 150);
  CHECK(decoded[1] == 0);
  CHECK(decoded[2] == 255);
  CHECK(decoded[3] == 255);
}

// ===========================================================================
// BGRA8888 interlaced -- wide round-trip (needs width > 256)
// ===========================================================================

TEST_CASE("BGRA8888 interlaced round-trips across 256-pixel column blocks") {
  constexpr int w = 258;
  constexpr int h = 2;
  std::vector<uint8_t> original(w * h * 4);
  for (int i = 0; i < w * h; ++i) {
    original[i * 4 + 0] = static_cast<uint8_t>(i & 0xFF);
    original[i * 4 + 1] = static_cast<uint8_t>((i >> 8) & 0xFF);
    original[i * 4 + 2] = static_cast<uint8_t>((i * 7) & 0xFF);
    original[i * 4 + 3] = static_cast<uint8_t>(200);
  }
  auto encoded = encode_rgba_to_bgra8888_interlaced(original, w, h);
  REQUIRE(encoded.size() == original.size());
  auto decoded = decode_bgra8888_interlaced_to_rgba(encoded, w, h);
  CHECK(rgba_eq(decoded, original));
}

// ===========================================================================
// decode_entry_to_png pipeline -- full round-trip via PNG
// ===========================================================================

TEST_CASE("decode_entry_to_png round-trips a BGRA8888 texture") {
  auto img = load_reference("test_nibble.png");
  auto pixels = encode_rgba_to_bgra8888(img.rgba, img.width, img.height);
  auto entry = make_texture_entry(img.width, img.height, 2, pixels);
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  REQUIRE(png);
  CHECK(image_eq(decode_png(png.value), img));
}

TEST_CASE("decode_entry_to_png round-trips a GBAR4444 texture") {
  auto img = load_reference("test_nibble.png");
  auto pixels = encode_rgba_to_gbar4444(img.rgba, img.width, img.height);
  auto entry = make_texture_entry(img.width, img.height, 0, pixels);
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  REQUIRE(png);
  CHECK(image_eq(decode_png(png.value), img));
}

TEST_CASE("decode_entry_to_png round-trips an Icon entry") {
  auto img = load_reference("test_nibble.png");
  auto pixels = encode_rgba_to_gbar4444(img.rgba, img.width, img.height);
  auto entry = make_icon_entry(img.width, img.height, pixels);
  auto png = decode_entry_to_png(entry, EntryType::Icon);
  REQUIRE(png);
  CHECK(image_eq(decode_png(png.value), img));
}

TEST_CASE("decode_entry_to_png round-trips a TileGrid entry") {
  auto img = load_reference("test_nstc.png");
  auto pixels = encode_rgba_to_nstc(img.rgba, img.width, img.height);
  auto entry = make_simple_entry(img.width, img.height, pixels);
  auto png = decode_entry_to_png(entry, EntryType::TileGrid);
  REQUIRE(png);
  CHECK(image_eq(decode_png(png.value), img));
}

TEST_CASE("decode_entry_to_png round-trips an Image4B entry") {
  auto img = load_reference("test_nibble.png");
  auto pixels = encode_rgba_to_bgra8888_interlaced(img.rgba, img.width, img.height);
  auto entry = make_simple_entry(img.width, img.height, pixels);
  auto png = decode_entry_to_png(entry, EntryType::Image4B);
  REQUIRE(png);
  CHECK(image_eq(decode_png(png.value), img));
}

// ===========================================================================
// Converter validation
// ===========================================================================

TEST_CASE("converters return empty on size mismatch") {
  const std::vector<uint8_t> too_small{0x00, 0x01};
  CHECK(decode_gbar4444_to_rgba(too_small, 2, 2).empty());
  CHECK(decode_bgra8888_to_rgba(too_small, 1, 1).empty());
  CHECK(decode_argb555_to_rgba(too_small, 2, 1).empty());
  CHECK(decode_nstc_to_rgba(too_small, 4, 1).empty());
  CHECK(decode_grayscale_to_rgba(too_small, 4, 1).empty());
  CHECK(decode_barg4444_to_rgba(too_small, 2, 1).empty());
}

TEST_CASE("converters reject non-positive dimensions") {
  const std::vector<uint8_t> pixels(8, 0x00);
  CHECK(decode_gbar4444_to_rgba(pixels, 0, 4).empty());
  CHECK(decode_gbar4444_to_rgba(pixels, 4, 0).empty());
  CHECK(decode_bgra8888_to_rgba(pixels, -1, 2).empty());
}

// ===========================================================================
// decode_entry_to_png -- negative cases
// ===========================================================================

TEST_CASE("decode_entry_to_png rejects non-image entry types") {
  const std::vector<uint8_t> data{0x01, 0x02, 0x03};
  require_rejects(data, EntryType::TextDat);
  require_rejects(data, EntryType::TextLst);
  require_rejects(data, EntryType::Unknown);
}

TEST_CASE("decode_entry_to_png rejects a truncated header") {
  require_rejects({0x02, 0x00, 0x02}, EntryType::Texture);
}

TEST_CASE("decode_entry_to_png rejects an unknown texture format byte") {
  require_rejects({0x02, 0x00, 0x01, 0x00, 9, 0, 0, 0, 0x40, 0x30, 0xF0, 0x0F}, EntryType::Texture);
}

TEST_CASE("decode_entry_to_png rejects pixel data shorter than declared dimensions") {
  require_rejects({0x02, 0x00, 0x02, 0x00, 2, 0, 0, 0, 10, 20, 30, 40, 50, 60, 70, 80},
                  EntryType::Texture);
}

// ===========================================================================
// Integration -- real fixture
// ===========================================================================

TEST_CASE("decode_entry_to_png extracts a PNG from a real NSipData.NOS icon") {
  auto path = ensure_fixture("NostaleData\\NSipData.NOS");
  auto result = NosArchive::open(path);
  REQUIRE(result);

  auto& archive = result.value;
  REQUIRE(archive.entries().size() > 0);

  bool decoded_any = false;
  for (const auto& entry : archive.entries()) {
    if (entry.type != EntryType::Icon) {
      continue;
    }
    auto data = archive.read_entry(static_cast<size_t>(&entry - archive.entries().data()));
    if (!data) {
      continue;
    }
    auto png = decode_entry_to_png(data.value, EntryType::Icon);
    if (!png) {
      continue;
    }

    decoded_any = true;
    auto img = decode_png(png.value);

    const int header_w = data.value[1] | data.value[2] << 8;
    const int header_h = data.value[3] | data.value[4] << 8;
    CHECK(static_cast<int>(img.width) == header_w);
    CHECK(static_cast<int>(img.height) == header_h);
    break;
  }

  CHECK(decoded_any);
}
