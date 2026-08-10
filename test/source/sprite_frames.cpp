#include <doctest/doctest.h>
#include <onex/archive/image/sprite_frames.h>
#include <onex/archive/nos_archive.h>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "fixture.h"

namespace {

  using onex::archive::decode_sprite_frames;
  using onex::archive::encode_sprite_frame_to_png;
  using onex::archive::SpriteFrame;

  auto load_entry(const std::string& archive_name, uint32_t id) -> std::vector<uint8_t> {
    auto path = ensure_fixture("NostaleData\\" + archive_name);
    auto opened = onex::archive::NosArchive::open(path);
    REQUIRE(opened);
    const auto& entries = opened.value.entries();
    for (size_t i = 0; i < entries.size(); ++i) {
      if (entries[i].id == id) {
        auto data = opened.value.read_entry(i);
        REQUIRE(data);
        return data.value;
      }
    }
    FAIL("entry not found in " << archive_name << ": " << id);
    return {};
  }

  auto decode(const std::string& archive_name, uint32_t id) -> std::vector<SpriteFrame> {
    auto data = load_entry(archive_name, id);
    auto frames = decode_sprite_frames(data);
    REQUIRE(frames);
    return std::move(frames.value);
  }

}  // namespace

TEST_CASE("decode_sprite_frames decodes the frames of NSmpData entry 59") {
  auto frames = decode("NSmpData00.NOS", 59);
  REQUIRE(frames.size() == 2);

  CHECK(frames[0].width == 46);
  CHECK(frames[0].height == 57);
  CHECK(frames[0].x_origin == 21);
  CHECK(frames[0].y_origin == 51);
  CHECK(frames[0].rgba.size() == static_cast<size_t>(46) * 57 * 4);

  CHECK(frames[1].width == 45);
  CHECK(frames[1].height == 57);
  CHECK(frames[1].x_origin == 22);
  CHECK(frames[1].y_origin == 51);
  CHECK(frames[1].rgba.size() == static_cast<size_t>(45) * 57 * 4);
}

TEST_CASE("decode_sprite_frames decodes NSmpData entries 60 and 61") {
  auto frames = decode("NSmpData00.NOS", 60);
  REQUIRE(frames.size() == 2);
  CHECK(frames[0].width == 92);
  CHECK(frames[0].height == 49);
  CHECK(frames[0].x_origin == 46);
  CHECK(frames[0].y_origin == 46);

  auto frames61 = decode("NSmpData00.NOS", 61);
  REQUIRE(frames61.size() == 2);
  CHECK(frames61[1].width == 47);
  CHECK(frames61[1].height == 66);
  CHECK(frames61[1].x_origin == 22);
  CHECK(frames61[1].y_origin == 54);
}

TEST_CASE("decode_sprite_frames decodes the frames of NSppData0B entry 22528") {
  auto frames = decode("NSppData0B.NOS", 22528);
  REQUIRE(frames.size() == 3);

  CHECK(frames[0].width == 41);
  CHECK(frames[0].height == 33);
  CHECK(frames[0].x_origin == 18);
  CHECK(frames[0].y_origin == 127);
  CHECK(frames[0].rgba.size() == static_cast<size_t>(41) * 33 * 4);

  CHECK(frames[1].width == 40);
  CHECK(frames[1].height == 34);
  CHECK(frames[1].y_origin == 128);
}

TEST_CASE("encode_sprite_frame_to_png produces a valid PNG") {
  auto frames = decode("NSmpData00.NOS", 59);
  REQUIRE(frames.size() == 2);

  auto png = encode_sprite_frame_to_png(frames[0]);
  REQUIRE(png);

  static constexpr std::array<uint8_t, 8> kPngMagic = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
  REQUIRE(png.value.size() > kPngMagic.size());
  CHECK(std::equal(kPngMagic.begin(), kPngMagic.end(), png.value.begin()));
}

TEST_CASE("decode_sprite_frames rejects truncated entry data") {
  auto data = load_entry("NSmpData00.NOS", 59);
  data.resize(data.size() - 1);
  auto frames = decode_sprite_frames(data);
  CHECK_FALSE(frames);
  CHECK(frames.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_sprite_frames rejects a truncated descriptor table") {
  std::vector<uint8_t> data{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // 2 frames, 1 descriptor
  auto frames = decode_sprite_frames(data);
  CHECK_FALSE(frames);
  CHECK(frames.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_sprite_frames decodes the first entries of NSppData0B") {
  auto path = ensure_fixture("NostaleData\\NSppData0B.NOS");
  auto opened = onex::archive::NosArchive::open(path);
  REQUIRE(opened);

  const auto& entries = opened.value.entries();
  const size_t limit = entries.size() < 100 ? entries.size() : 100;
  for (size_t i = 0; i < limit; ++i) {
    auto data = opened.value.read_entry(i);
    REQUIRE(data);
    auto frames = decode_sprite_frames(data.value);
    REQUIRE(frames);
    for (const auto& frame : frames.value) {
      CHECK(frame.rgba.size() == static_cast<size_t>(frame.width) * frame.height * 4);
    }
  }
}
