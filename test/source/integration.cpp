#include <doctest/doctest.h>
#include <lodepng.h>
#include <onex/archive/effect/effect_definition.h>
#include <onex/archive/image/sprite_frames.h>
#include <onex/archive/nos_archive.h>
#include <onex/archive/sprite/sprite_info.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "fixture.h"

namespace {

  using onex::archive::decode_effect_definition;
  using onex::archive::decode_sprite_frames;
  using onex::archive::decode_sprite_info;
  using onex::archive::encode_sprite_frame_to_png;
  using onex::archive::NosArchive;
  using onex::archive::SpriteFrame;
  using onex::archive::SpriteVariant;

  auto open_archive(const std::string& name) -> NosArchive {
    auto path = ensure_fixture("NostaleData\\" + name);
    auto opened = onex::archive::NosArchive::open(path);
    REQUIRE(opened);
    return std::move(opened.value);
  }

  auto load_entry(NosArchive& archive, uint32_t id) -> std::vector<uint8_t> {
    const auto& entries = archive.entries();
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].id == id) {
        auto data = archive.read_entry(i);
        REQUIRE(data);
        return data.value;
      }
    }
    FAIL("entry not found: " << id);
    return {};
  }

}  // namespace

TEST_CASE("sprite extraction round-trips pixel data through PNG") {
  // Encoding a decoded frame with fpng and decoding the result with lodepng
  // (an independent PNG decoder) must reproduce the exact RGBA bytes. This
  // proves the GBAR4444 -> RGBA -> PNG pipeline produces correct images.
  struct Sample {
    const char* archive;
    uint32_t id;
  };
  const std::vector<Sample> samples = {
      {"NSmpData00.NOS", 59},
      {"NSmpData00.NOS", 60},
      {"NSmpData00.NOS", 61},
      {"NSppData0B.NOS", 22528},
  };

  for (const auto& sample : samples) {
    CAPTURE(sample.archive);
    CAPTURE(sample.id);
    auto archive = open_archive(sample.archive);
    auto entry_data = load_entry(archive, sample.id);
    auto frames = decode_sprite_frames(entry_data);
    REQUIRE(frames);
    REQUIRE_FALSE(frames.value.empty());

    for (size_t i = 0; i < frames.value.size(); ++i) {
      const SpriteFrame& frame = frames.value[i];
      auto png = encode_sprite_frame_to_png(frame);
      REQUIRE(png);

      std::vector<unsigned char> decoded;
      unsigned width = 0;
      unsigned height = 0;
      const unsigned error
          = lodepng::decode(decoded, width, height, png.value.data(), png.value.size());
      CHECK(error == 0);
      CHECK(static_cast<int>(width) == frame.width);
      CHECK(static_cast<int>(height) == frame.height);
      CHECK(decoded.size() == frame.rgba.size());
      CHECK(decoded == frame.rgba);
      if (error != 0 || decoded.size() != frame.rgba.size()) {
        MESSAGE("round-trip mismatch: entry " << sample.id << " frame " << i);
      }

      // The frame must contain actual pixels, not an empty/transparent buffer.
      bool has_opaque_pixel = false;
      for (size_t p = 3; p < frame.rgba.size(); p += 4) {
        if (frame.rgba[p] != 0) {
          has_opaque_pixel = true;
          break;
        }
      }
      CHECK_MESSAGE(has_opaque_pixel,
                    "frame " << i << " of entry " << sample.id << " is fully transparent");
    }
  }
}

TEST_CASE("sprite, sprite info, and effect decoding agree across formats") {
  // A single end-to-end pass over one archive of each kind: every entry of a
  // small CCINF archive, the first entries of a sprite archive, and one effect
  // archive entry must decode and (for sprites) encode without failing.
  auto info_archive = open_archive("NSpnData.NOS");
  for (const auto& entry : info_archive.entries()) {
    auto data
        = info_archive.read_entry(static_cast<size_t>(&entry - info_archive.entries().data()));
    REQUIRE(data);
    auto doc = decode_sprite_info(data.value, SpriteVariant::kPlayer);
    REQUIRE(doc);
    CHECK(doc.value["parts"].size() == 7);
  }

  auto sprite_archive = open_archive("NSppData0B.NOS");
  const size_t limit = sprite_archive.entries().size() < 50 ? sprite_archive.entries().size() : 50;
  for (size_t i = 0; i < limit; ++i) {
    auto data = sprite_archive.read_entry(i);
    REQUIRE(data);
    auto frames = decode_sprite_frames(data.value);
    REQUIRE(frames);
    for (const auto& frame : frames.value) {
      auto png = encode_sprite_frame_to_png(frame);
      REQUIRE(png);
      CHECK(png.value.size() > 8);
    }
  }

  auto effect_archive = open_archive("NSeffData.NOS");
  auto effect_data = load_entry(effect_archive, 0);
  auto doc = decode_effect_definition(effect_data);
  REQUIRE(doc);
  CHECK(doc.value["components"].size() == 5);
}
