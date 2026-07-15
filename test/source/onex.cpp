#include <doctest/doctest.h>
#include <onex/version.h>

#include <string>
#include <string_view>

TEST_CASE("OnexExplorer version") {
  static_assert(std::string_view(ONEXEXPLORER_VERSION) == std::string_view("0.0.0"));
  CHECK(std::string(ONEXEXPLORER_VERSION) == std::string("0.0.0"));
}
