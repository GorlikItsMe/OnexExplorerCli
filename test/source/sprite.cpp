#include <doctest/doctest.h>
#include <onex/archive/nos_archive.h>
#include <onex/archive/sprite/sprite_info.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "fixture.h"

namespace {

  using onex::archive::decode_sprite_info;
  using onex::archive::NosArchive;
  using onex::archive::SpriteVariant;

  auto open_archive(const std::string& name) -> NosArchive {
    auto path = ensure_fixture("NostaleData\\" + name);
    auto opened = onex::archive::NosArchive::open(path);
    REQUIRE(opened);
    return std::move(opened.value);
  }

  auto decode(NosArchive& archive, size_t index, SpriteVariant variant) -> nlohmann::json {
    auto data = archive.read_entry(index);
    REQUIRE(data);
    auto doc = decode_sprite_info(data.value, variant);
    REQUIRE(doc);
    return doc.value;
  }

}  // namespace

TEST_CASE("decode_sprite_info decodes the monster variant header of NSmnData entry 0") {
  auto archive = open_archive("NSmnData.NOS");
  auto doc = decode(archive, 0, SpriteVariant::kMonster);

  CHECK(doc["direction"] == 0);
  CHECK(doc["animation"] == 1);
  CHECK(doc["monster"] == 0);
  CHECK(doc["base"] == 59);
  CHECK(doc["nspm"] == -1);
  CHECK(doc["kit"] == 0);
  CHECK(doc["parts"].size() == 7);
  for (int i = 0; i < 7; ++i) {
    CHECK(doc["parts"][std::to_string(i)].empty());
  }
}

TEST_CASE("decode_sprite_info decodes monster variant part lists") {
  auto archive = open_archive("NSmnData.NOS");
  auto doc = decode(archive, 1627, SpriteVariant::kMonster);

  CHECK(doc["direction"] == 0);
  CHECK(doc["animation"] == 11);
  CHECK(doc["monster"] == 166);
  CHECK(doc["base"] == 1685);
  CHECK(doc["nspm"] == -1);
  CHECK(doc["kit"] == 178);
  CHECK(doc["parts"]["0"].size() == 1);
  CHECK(doc["parts"]["0"][0]["index"] == 0);
  CHECK(doc["parts"]["0"][0]["id"] == 10540);
}

TEST_CASE("decode_sprite_info decodes the player variant header and named parts") {
  auto archive = open_archive("NSpnData.NOS");
  auto doc = decode(archive, 0, SpriteVariant::kPlayer);

  CHECK(doc["direction"] == 0);
  CHECK(doc["animation"] == 0);
  CHECK(doc["sex"] == 0);
  CHECK(doc["class"] == 0);
  CHECK(doc["morph"] == 0);
  CHECK(doc["base"] == 3997);
  CHECK(doc["nspm"] == 0);
  CHECK(doc["kit"] == 0);

  CHECK(doc["parts"]["armor"].size() == 3);
  CHECK(doc["parts"]["armor"][0]["index"] == 0);
  CHECK(doc["parts"]["armor"][0]["id"] == 35742);
  CHECK(doc["parts"]["armor"][2]["index"] == 2);
  CHECK(doc["parts"]["armor"][2]["id"] == 35868);
  CHECK(doc["parts"]["hair"].size() == 2);
  CHECK(doc["parts"]["hair"][1]["id"] == 33976);
  CHECK(doc["parts"]["hat"].size() == 4);
  CHECK(doc["parts"]["hat"][2]["id"] == 13211);
  CHECK(doc["parts"]["hat"][3]["index"] == 130);
  CHECK(doc["parts"]["hat"][3]["id"] == 166315);
  CHECK(doc["parts"]["mask"].size() == 1);
  CHECK(doc["parts"]["mask"][0]["index"] == 1);
  CHECK(doc["parts"]["mask"][0]["id"] == 7137);
  CHECK(doc["parts"]["secondary-Weapon"].empty());
  CHECK(doc["parts"]["first-Weapon"].size() == 4);
  CHECK(doc["parts"]["first-Weapon"][1]["index"] == 1);
  CHECK(doc["parts"]["first-Weapon"][1]["id"] == 206);
  CHECK(doc["parts"]["first-Weapon"][3]["index"] == 3);
  CHECK(doc["parts"]["first-Weapon"][3]["id"] == 280);
  CHECK(doc["parts"]["weapon-part"].empty());
}

TEST_CASE("decode_sprite_info decodes player sex and class from the packed byte") {
  auto archive = open_archive("NSpnData.NOS");
  auto doc = decode(archive, 415, SpriteVariant::kPlayer);

  CHECK(doc["sex"] == 1);
  CHECK(doc["class"] == 1);
  CHECK(doc["morph"] == 0);
  CHECK(doc["base"] == 4320);
  CHECK(doc["nspm"] == 48);
  CHECK(doc["kit"] == 16);
}

TEST_CASE("decode_sprite_info rejects truncated data") {
  auto archive = open_archive("NSpnData.NOS");
  auto data = archive.read_entry(0);
  REQUIRE(data);
  data.value.resize(data.value.size() - 1);
  auto doc = decode_sprite_info(data.value, SpriteVariant::kPlayer);
  CHECK_FALSE(doc);
  CHECK(doc.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_sprite_info rejects trailing garbage") {
  auto archive = open_archive("NSpnData.NOS");
  auto data = archive.read_entry(0);
  REQUIRE(data);
  data.value.push_back(0x00);
  auto doc = decode_sprite_info(data.value, SpriteVariant::kPlayer);
  CHECK_FALSE(doc);
  CHECK(doc.error == onex::Error::kInvalidFormat);
}

TEST_CASE("decode_sprite_info decodes every entry of both CCINF fixtures") {
  auto archive = open_archive("NSmnData.NOS");
  for (size_t i = 0; i < archive.entries().size(); ++i) {
    auto data = archive.read_entry(i);
    REQUIRE(data);
    auto doc = decode_sprite_info(data.value, SpriteVariant::kMonster);
    REQUIRE(doc);
    CHECK(doc.value["parts"].size() == 7);
  }

  auto player_archive = open_archive("NSpnData.NOS");
  for (size_t i = 0; i < player_archive.entries().size(); ++i) {
    auto data = player_archive.read_entry(i);
    REQUIRE(data);
    auto doc = decode_sprite_info(data.value, SpriteVariant::kPlayer);
    REQUIRE(doc);
    CHECK(doc.value["parts"].size() == 7);
  }
}
