#include <doctest/doctest.h>
#include <lodepng.h>
#include <onex/archive/entry.h>
#include <onex/archive/image/converter.h>
#include <onex/archive/image/entry_image.h>
#include <onex/archive/nos_archive.h>

#include <cstring>
#include <vector>

#include "fixture.h"

using namespace onex::archive;

namespace {

  auto rgba_eq(std::span<const uint8_t> actual, std::span<const uint8_t> expected) -> bool {
    return actual.size() == expected.size()
           && std::memcmp(actual.data(), expected.data(), actual.size()) == 0;
  }

}  // namespace

// ---------------------------------------------------------------------------
// GBAR4444 -- known pattern decode
// ---------------------------------------------------------------------------

TEST_CASE("decode_gbar4444_to_rgba produces correct RGBA for a 2x2 pattern") {
  // gb,ar pairs: g=gb>>4, b=gb&0xF, a=ar>>4, r=ar&0xF; channel = nibble*0x11
  const std::vector<uint8_t> pixels{
      0x40, 0x30,  // P0: g=4,b=0,a=3,r=0
      0xF0, 0x0F,  // P1: g=15,b=0,a=0,r=15
      0x0F, 0xF0,  // P2: g=0,b=15,a=15,r=0
      0xAB, 0xCD,  // P3: g=10,b=11,a=12,r=13
  };
  const std::vector<uint8_t> expected{
      0x00, 0x44, 0x00, 0x33,  // P0
      0xFF, 0xFF, 0x00, 0x00,  // P1
      0x00, 0x00, 0xFF, 0xFF,  // P2
      0xDD, 0xAA, 0xBB, 0xCC,  // P3
  };

  auto rgba = decode_gbar4444_to_rgba(pixels, 2, 2);
  CHECK(rgba_eq(rgba, expected));
}

// ---------------------------------------------------------------------------
// BGRA8888 -- known pattern decode
// ---------------------------------------------------------------------------

TEST_CASE("decode_bgra8888_to_rgba swaps B and R channels") {
  const std::vector<uint8_t> pixels{
      10, 20, 30, 40,  // b,g,r,a
      50, 60, 70, 80,
  };
  const std::vector<uint8_t> expected{
      30, 20, 10, 40,  // r,g,b,a
      70, 60, 50, 80,
  };

  auto rgba = decode_bgra8888_to_rgba(pixels, 2, 1);
  CHECK(rgba_eq(rgba, expected));
}

// ---------------------------------------------------------------------------
// ARGB555 -- known pattern decode
// ---------------------------------------------------------------------------

TEST_CASE("decode_argb555_to_rgba expands 5-bit channels and 1-bit alpha") {
  // uint16 little-endian: 0x8000 = fully transparent... wait, bit15 set = opaque
  const std::vector<uint8_t> pixels{
      0x00,
      0x80,  // 0x8000: a=255, r=g=b=0
      0xFF,
      0x7F,  // 0x7FFF: a=0, r=g=b=248
  };
  const std::vector<uint8_t> expected{
      0,   0,   0,   255,  // P0: opaque black
      248, 248, 248, 0,    // P1: transparent white-ish
  };

  auto rgba = decode_argb555_to_rgba(pixels, 2, 1);
  CHECK(rgba_eq(rgba, expected));
}

// ---------------------------------------------------------------------------
// NSTC -- known pattern decode with palette lookup and fallback
// ---------------------------------------------------------------------------

TEST_CASE("decode_nstc_to_rgba maps indices through the palette") {
  const std::vector<uint8_t> pixels{
      0x00,  // white
      0x03,  // black
      0xFF,  // palette fallback colour
      0x05,  // undefined index -> fallback (150,0,255)
  };
  const std::vector<uint8_t> expected{
      255, 255, 255, 255,  // 0x00
      0,   0,   0,   255,  // 0x03
      150, 0,   255, 255,  // 0xFF
      150, 0,   255, 255,  // 0x05 -> fallback
  };

  auto rgba = decode_nstc_to_rgba(pixels, 2, 2);
  CHECK(rgba_eq(rgba, expected));
}

TEST_CASE("encode_rgba_to_nstc maps palette colours back and 0xFF for others") {
  const std::vector<uint8_t> rgba{
      255, 255, 255, 255,  // -> 0x00
      0,   0,   0,   255,  // -> 0x03
      150, 0,   255, 255,  // -> 0xFF
      123, 45,  67,  255,  // not in palette -> 0xFF
  };
  const std::vector<uint8_t> expected{0x00, 0x03, 0xFF, 0xFF};

  auto pixels = encode_rgba_to_nstc(rgba, 2, 2);
  CHECK(rgba_eq(pixels, expected));
}

// ---------------------------------------------------------------------------
// BGRA8888 interlaced -- byte swap (small) + reordering (wide round-trip)
// ---------------------------------------------------------------------------

TEST_CASE("decode_bgra8888_interlaced_to_rgba swaps channels for small image") {
  // width <= 256: the interlaced traversal is plain row-major.
  const std::vector<uint8_t> pixels{
      10, 20, 30, 40,  // b,g,r,a
      50, 60, 70, 80,
  };
  const std::vector<uint8_t> expected{
      30, 20, 10, 40,  // r,g,b,a
      70, 60, 50, 80,
  };

  auto rgba = decode_bgra8888_interlaced_to_rgba(pixels, 2, 1);
  CHECK(rgba_eq(rgba, expected));
}

TEST_CASE("BGRA8888 interlaced round-trips across 256-pixel column blocks") {
  // width > 256 forces a second column block, exercising the reordering.
  constexpr int w = 258;
  constexpr int h = 2;
  std::vector<uint8_t> original(w * h * 4);
  for (int i = 0; i < w * h; ++i) {
    original[i * 4 + 0] = static_cast<uint8_t>(i & 0xFF);         // r
    original[i * 4 + 1] = static_cast<uint8_t>((i >> 8) & 0xFF);  // g
    original[i * 4 + 2] = static_cast<uint8_t>((i * 7) & 0xFF);   // b
    original[i * 4 + 3] = static_cast<uint8_t>(200);              // a
  }

  auto encoded = encode_rgba_to_bgra8888_interlaced(original, w, h);
  REQUIRE(encoded.size() == original.size());

  auto decoded = decode_bgra8888_interlaced_to_rgba(encoded, w, h);
  CHECK(rgba_eq(decoded, original));
}

// ---------------------------------------------------------------------------
// BARG4444 -- known pattern decode
// ---------------------------------------------------------------------------

TEST_CASE("decode_barg4444_to_rgba produces correct RGBA") {
  // ba,rg pairs: b=ba>>4, a=ba&0xF, r=rg>>4, g=rg&0xF
  const std::vector<uint8_t> pixels{
      0x40,
      0x30,  // P0: b=4,a=0,r=3,g=0
      0xF0,
      0x0F,  // P1: b=15,a=0,r=0,g=15
  };
  const std::vector<uint8_t> expected{
      0x33, 0x00, 0x44, 0x00,  // P0
      0x00, 0xFF, 0xFF, 0x00,  // P1
  };

  auto rgba = decode_barg4444_to_rgba(pixels, 2, 1);
  CHECK(rgba_eq(rgba, expected));
}

// ---------------------------------------------------------------------------
// Grayscale -- known pattern decode
// ---------------------------------------------------------------------------

TEST_CASE("decode_grayscale_to_rgba replicates luminance to all channels") {
  const std::vector<uint8_t> pixels{100, 200};
  const std::vector<uint8_t> expected{
      100, 100, 100, 100, 200, 200, 200, 200,
  };

  auto rgba = decode_grayscale_to_rgba(pixels, 2, 1);
  CHECK(rgba_eq(rgba, expected));
}

// ---------------------------------------------------------------------------
// Round-trip tests for lossless formats
// ---------------------------------------------------------------------------

TEST_CASE("GBAR4444 round-trips when channels are multiples of 0x11") {
  const std::vector<uint8_t> rgba{
      0x00, 0x44, 0x00, 0x33, 0xFF, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0xFF, 0xFF, 0xDD, 0xAA, 0xBB, 0xCC,
  };
  auto encoded = encode_rgba_to_gbar4444(rgba, 2, 2);
  REQUIRE(encoded.size() == 8);
  auto decoded = decode_gbar4444_to_rgba(encoded, 2, 2);
  CHECK(rgba_eq(decoded, rgba));
}

TEST_CASE("BGRA8888 round-trips arbitrary RGBA") {
  const std::vector<uint8_t> rgba{
      10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160,
  };
  auto encoded = encode_rgba_to_bgra8888(rgba, 2, 2);
  REQUIRE(encoded.size() == 16);
  auto decoded = decode_bgra8888_to_rgba(encoded, 2, 2);
  CHECK(rgba_eq(decoded, rgba));
}

TEST_CASE("BARG4444 round-trips when channels are multiples of 0x11") {
  const std::vector<uint8_t> rgba{
      0x33, 0x00, 0x44, 0x00, 0x00, 0xFF, 0xFF, 0x00,
  };
  auto encoded = encode_rgba_to_barg4444(rgba, 2, 1);
  REQUIRE(encoded.size() == 4);
  auto decoded = decode_barg4444_to_rgba(encoded, 2, 1);
  CHECK(rgba_eq(decoded, rgba));
}

TEST_CASE("Grayscale round-trips when R=G=B=A") {
  const std::vector<uint8_t> rgba{
      100, 100, 100, 100, 200, 200, 200, 200,
  };
  auto encoded = encode_rgba_to_grayscale(rgba, 2, 1);
  REQUIRE(encoded.size() == 2);
  auto decoded = decode_grayscale_to_rgba(encoded, 2, 1);
  CHECK(rgba_eq(decoded, rgba));
}

// ---------------------------------------------------------------------------
// Converter size validation
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// decode_entry_to_png -- PNG validity
// ---------------------------------------------------------------------------

TEST_CASE("decode_entry_to_png produces a valid PNG for a BGRA8888 texture") {
  // 8-byte texture header: width=2, height=2, format=2 (BGRA8888), then pixels.
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x02, 0x00, 2, 0, 0, 0,  // header
      10,   20,   30,   40,                // P0 bgra
      50,   60,   70,   80,                // P1 bgra
      90,   100,  110,  120,               // P2 bgra
      130,  140,  150,  160,               // P3 bgra
  };

  auto png = decode_entry_to_png(entry, EntryType::Texture);
  REQUIRE(png);
  REQUIRE(png.value.size() >= 8);

  // PNG signature: 0x89 P N G \r \n 0x1A \n
  const std::vector<uint8_t> signature{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  CHECK(std::memcmp(png.value.data(), signature.data(), 8) == 0);

  // lodepng must decode it back to the expected RGBA.
  std::vector<unsigned char> rgba;
  unsigned w = 0;
  unsigned h = 0;
  REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);
  CHECK(w == 2);
  CHECK(h == 2);

  const std::vector<uint8_t> expected_rgba{
      30, 20, 10, 40, 70, 60, 50, 80, 110, 100, 90, 120, 150, 140, 130, 160,
  };
  CHECK(rgba.size() == expected_rgba.size());
  CHECK(std::memcmp(rgba.data(), expected_rgba.data(), rgba.size()) == 0);
}

TEST_CASE("decode_entry_to_png selects format from the texture format byte") {
  // format=0 (GBAR4444): 2 bytes per pixel.
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x01, 0x00, 0, 0, 0, 0,  // 2x1, GBAR4444
      0x40, 0x30, 0xF0, 0x0F,              // 2 pixels
  };
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  REQUIRE(png);

  std::vector<unsigned char> rgba;
  unsigned w = 0;
  unsigned h = 0;
  REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);
  CHECK(w == 2);
  CHECK(h == 1);
  const std::vector<uint8_t> expected{0x00, 0x44, 0x00, 0x33, 0xFF, 0xFF, 0x00, 0x00};
  CHECK(std::memcmp(rgba.data(), expected.data(), rgba.size()) == 0);
}

TEST_CASE("decode_entry_to_png decodes an Icon entry with a 13-byte header") {
  // Icon header: type(1), width(2)@1, height(2)@3, then 8 bytes of center/unknown,
  // then GBAR4444 pixels.
  const std::vector<uint8_t> entry{
      0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x40, 0x30, 0xF0, 0x0F,
  };
  auto png = decode_entry_to_png(entry, EntryType::Icon);
  REQUIRE(png);

  std::vector<unsigned char> rgba;
  unsigned w = 0;
  unsigned h = 0;
  REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);
  CHECK(w == 2);
  CHECK(h == 1);
}

TEST_CASE("decode_entry_to_png decodes a TileGrid entry with a 4-byte header") {
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x02, 0x00,  // 2x2
      0x00, 0x03, 0xFF, 0xFF,  // 4 NSTC indices
  };
  auto png = decode_entry_to_png(entry, EntryType::TileGrid);
  REQUIRE(png);

  std::vector<unsigned char> rgba;
  unsigned w = 0;
  unsigned h = 0;
  REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);
  CHECK(w == 2);
  CHECK(h == 2);
}

TEST_CASE("decode_entry_to_png decodes an Image4B entry (interlaced BGRA8888)") {
  // 4-byte header (width=2, height=1) + 2 interlaced BGRA pixels. width<=256
  // so the layout is plain row-major.
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x01, 0x00, 10, 20, 30, 40, 50, 60, 70, 80,
  };
  auto png = decode_entry_to_png(entry, EntryType::Image4B);
  REQUIRE(png);

  std::vector<unsigned char> rgba;
  unsigned w = 0;
  unsigned h = 0;
  REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);
  CHECK(w == 2);
  CHECK(h == 1);
  const std::vector<uint8_t> expected{30, 20, 10, 40, 70, 60, 50, 80};
  CHECK(std::memcmp(rgba.data(), expected.data(), rgba.size()) == 0);
}

// ---------------------------------------------------------------------------
// decode_entry_to_png -- negative cases
// ---------------------------------------------------------------------------

TEST_CASE("decode_entry_to_png rejects non-image entry types") {
  const std::vector<uint8_t> data{0x01, 0x02, 0x03};
  CHECK_FALSE(decode_entry_to_png(data, EntryType::TextDat));
  CHECK(decode_entry_to_png(data, EntryType::TextDat).error == onex::Error::kInvalidFormat);
  CHECK_FALSE(decode_entry_to_png(data, EntryType::TextLst));
  CHECK_FALSE(decode_entry_to_png(data, EntryType::Unknown));
}

TEST_CASE("decode_entry_to_png rejects a truncated texture header") {
  const std::vector<uint8_t> truncated{0x02, 0x00, 0x02};
  auto png = decode_entry_to_png(truncated, EntryType::Texture);
  CHECK_FALSE(png);
  CHECK(png.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_entry_to_png rejects an unknown texture format byte") {
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x01, 0x00, 9, 0, 0, 0,  // format=9 (unsupported)
      0x40, 0x30, 0xF0, 0x0F,
  };
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  CHECK_FALSE(png);
  CHECK(png.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_entry_to_png rejects pixel data shorter than declared dimensions") {
  // 2x2 BGRA8888 needs 16 pixel bytes; only provide 8.
  const std::vector<uint8_t> entry{
      0x02, 0x00, 0x02, 0x00, 2, 0, 0, 0, 10, 20, 30, 40, 50, 60, 70, 80,
  };
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  CHECK_FALSE(png);
  CHECK(png.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_entry_to_png rejects zero-dimension image") {
  const std::vector<uint8_t> entry{0x00, 0x00, 0x00, 0x00, 2, 0, 0, 0};
  auto png = decode_entry_to_png(entry, EntryType::Texture);
  CHECK_FALSE(png);
  CHECK(png.error == onex::Error::kInvalidFormat);
}

// ---------------------------------------------------------------------------
// Integration -- real fixture
// ---------------------------------------------------------------------------

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
    REQUIRE(png.value.size() >= 8);
    CHECK(png.value[0] == 0x89);
    CHECK(png.value[1] == 'P');

    std::vector<unsigned char> rgba;
    unsigned w = 0;
    unsigned h = 0;
    REQUIRE(lodepng::decode(rgba, w, h, png.value) == 0);

    const int header_w = data.value[1] | data.value[2] << 8;
    const int header_h = data.value[3] | data.value[4] << 8;
    CHECK(static_cast<int>(w) == header_w);
    CHECK(static_cast<int>(h) == header_h);
    break;
  }

  CHECK(decoded_any);
}
