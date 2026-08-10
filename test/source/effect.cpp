#include <doctest/doctest.h>
#include <onex/archive/effect/effect_definition.h>
#include <onex/archive/nos_archive.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <span>
#include <vector>

#include "fixture.h"

namespace {

  // Reads and decompresses the entry with the given id from the NSeffData
  // fixture archive. Fails the test when the fixture cannot be resolved.
  auto load_entry(uint32_t id) -> std::vector<uint8_t> {
    auto path = ensure_fixture("NostaleData\\NSeffData.NOS");
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
    FAIL("entry not found in NSeffData fixture: " << id);
    return {};
  }

  auto decode(uint32_t id) -> nlohmann::json {
    auto data = load_entry(id);
    auto doc = onex::archive::decode_effect_definition(data);
    REQUIRE(doc);
    return doc.value;
  }

}  // namespace

TEST_CASE("decode_effect_definition decodes the root header of entry 0") {
  auto doc = decode(0);
  CHECK(doc["resource_key"] == 0);
  CHECK(doc["components"].size() == 5);
  CHECK(doc["loader_workspace"].size() == 5);
  CHECK(doc["loader_workspace"].contains("loaded_tick_slot"));
  CHECK(doc["loader_workspace"].contains("source_record_slot"));
  CHECK(doc["loader_workspace"].contains("child_records_slot"));
  CHECK(doc["loader_workspace"].contains("reference_count_slot"));
  CHECK(doc["loader_workspace"].contains("flags_slot"));
}

TEST_CASE("decode_effect_definition decodes the positioned component of entry 0") {
  auto doc = decode(0);
  const auto& props = doc["components"][0]["properties"];
  CHECK(props["orientation_mode"] == 1);
  CHECK(props["timeline_mode"] == 2);
  CHECK(props["record_timeline_value"] == 0);
  CHECK(props["source_blend_factor"] == 768);
  CHECK(props["destination_blend_factor"] == 1);
  CHECK(props["geometry_resource_key"] == 1325465636);
  CHECK(props["callback_value"] == -1);
  CHECK(props["animation_timing"]["first_key"] == 0);
  CHECK(props["animation_timing"]["last_key"] == 40);
  CHECK(props["animation_timing"]["rate"] == 100);
  CHECK(props["animation_timing"]["units_per_key"] == 320);
  CHECK(props["start_offset_ms"] == 0);
  CHECK(props["duration_or_loop_ms"] == 100);
  CHECK(props["loop_animation"] == 0);
  CHECK(props["base_scale"] == 1);
  CHECK(props["transform_animation_enabled"] == 1);
  CHECK(props["variant"]["kind"] == "positioned");
  CHECK(props["variant"]["placement_mode"] == 1);
  CHECK(props["variant"]["source_height_scale"].get<double>() == doctest::Approx(0.6));
  CHECK(props["variant"]["use_target_height"] == 0);
}

TEST_CASE("decode_effect_definition decodes the particle variant of entry 0") {
  auto doc = decode(0);
  const auto& particle = doc["components"][2]["properties"]["variant"];
  CHECK(particle["kind"] == "particle");
  CHECK(particle["flags"] == 128);
  CHECK(particle["spawn_offset"].size() == 3);
  CHECK(particle["spawn_offset"][0] == 0.0);
  CHECK(particle["spawn_offset"][1].get<double>() == doctest::Approx(0.5));
  CHECK(particle["spawn_offset"][2] == 0.0);
  CHECK(particle["rotation"].size() == 4);
  CHECK(particle["rotation"][3] == 32767);
  CHECK(particle["axis_random_range"].size() == 3);
  CHECK(particle["particle_size"][0].get<double>() == doctest::Approx(3.0));
  CHECK(particle["axis_randomization"] == nlohmann::json::array({20, 30, 30}));
  CHECK(particle["gravity_factor"] == 120);
  CHECK(particle["initial_spawn_count"] == 6);
  CHECK(particle["spawn_delay_base_ms"] == 50);
  CHECK(particle["particle_lifetime_base_ms"] == 200);
  CHECK(particle["particle_lifetime_random_range_ms"] == 200);
}

TEST_CASE("decode_effect_definition decodes the flying variant of entry 1") {
  auto doc = decode(1);
  const auto& flying = doc["components"][0]["properties"]["variant"];
  CHECK(flying["kind"] == "flying");
  CHECK(flying["placement_mode"] == 0);
  CHECK(flying["target_height_scale"].get<double>() == doctest::Approx(0.6));
  CHECK(flying["target_offset"] == nlohmann::json::array({0.0, 0.0, 0.0}));
  CHECK(flying["fade_out_distance"].get<double>() == doctest::Approx(0.5));
  CHECK(flying["travel_rate"].get<double>() == doctest::Approx(10.0));
  CHECK(flying["sine_height_scale"].get<double>() == doctest::Approx(0.05));
}

TEST_CASE("decode_effect_definition decodes track keyframes of entry 0") {
  auto doc = decode(0);
  const auto& comp0 = doc["components"][0];

  const auto& scale = comp0["transform"]["scale"];
  CHECK(scale.size() == 2);
  CHECK(scale[0]["time"] == 0);
  CHECK(scale[0]["value"] == nlohmann::json::array({1.0, 1.0, 1.0}));
  CHECK(scale[1]["time"] == 2000);
  CHECK(scale[1]["value"] == nlohmann::json::array({2.0, 2.0, 2.0}));

  const auto& translation = comp0["transform"]["translation"];
  CHECK(translation.size() == 1);
  CHECK(translation[0]["time"] == 0);
  CHECK(translation[0]["value"].size() == 3);
  CHECK(translation[0]["value"][0].get<double>() == doctest::Approx(0.0));
  CHECK(translation[0]["value"][1].get<double>() == doctest::Approx(0.1));
  CHECK(translation[0]["value"][2].get<double>() == doctest::Approx(0.5));

  CHECK(comp0["transform"]["rotation"].empty());

  const auto& color = comp0["color"];
  CHECK(color.size() == 3);
  CHECK(color[0]["time"] == 0);
  CHECK(color[0]["color"]["red"] == 181);
  CHECK(color[0]["color"]["green"] == 106);
  CHECK(color[0]["color"]["blue"] == 15);
  CHECK(color[0]["color"]["alpha"] == 255);
  CHECK(color[2]["time"] == 9000);
  CHECK(color[2]["color"]["alpha"] == 0);

  const auto& texture = comp0["texture"];
  CHECK(texture.size() == 1);
  CHECK(texture[0]["time"] == 0);
  CHECK(texture[0]["texture_resource_key"] == 1325400096);

  CHECK(comp0["texture_transform"]["scale"].empty());
  CHECK(comp0["texture_transform"]["translation"].empty());
}

TEST_CASE("decode_effect_definition decodes texture_transform tracks of entry 18") {
  auto doc = decode(18);
  const auto& comp0 = doc["components"][0];
  CHECK(comp0["transform"]["rotation"].size() >= 1);
  CHECK(comp0["transform"]["rotation"][0]["time"] == 0);
  CHECK(comp0["transform"]["rotation"][0]["value"].size() == 4);
}

TEST_CASE("decode_effect_definition decodes every entry in the NSeffData fixture") {
  auto path = ensure_fixture("NostaleData\\NSeffData.NOS");
  auto opened = onex::archive::NosArchive::open(path);
  REQUIRE(opened);
  auto& archive = opened.value;
  auto count = archive.entries().size();
  int decoded = 0;
  for (size_t i = 0; i < count; ++i) {
    auto data = archive.read_entry(i);
    REQUIRE(data);
    auto doc = onex::archive::decode_effect_definition(data.value);
    if (!doc) {
      FAIL("decode failed for entry " << i);
    }
    ++decoded;
  }
  CHECK(decoded == static_cast<int>(count));
}

TEST_CASE("decode_effect_definition returns kInvalidFormat for truncated bodies") {
  auto data = load_entry(0);
  auto doc = onex::archive::decode_effect_definition(std::span(data).first(100));
  CHECK_FALSE(doc);
  CHECK(doc.error == onex::Error::kInvalidFormat);

  std::vector<uint8_t> tiny(10, 0);
  auto doc2 = onex::archive::decode_effect_definition(tiny);
  CHECK_FALSE(doc2);
  CHECK(doc2.error == onex::Error::kInvalidFormat);
}
