#include <doctest/doctest.h>
#include <onexexplorer/onexplorer.h>
#include <onexexplorer/version.h>

#include <string>
#include <string_view>
TEST_CASE("OnexExplorer") {
  using namespace onexexplorer;

  OnexExplorer cli("Tests");

  CHECK(cli.greet(LanguageCode::EN) == "Hello, Tests!");
  CHECK(cli.greet(LanguageCode::DE) == "Hallo Tests!");
  CHECK(cli.greet(LanguageCode::ES) == "¡Hola Tests!");
  CHECK(cli.greet(LanguageCode::FR) == "Bonjour Tests!");
}

TEST_CASE("OnexExplorer version") {
  static_assert(std::string_view(ONEXEXPLORER_VERSION) == std::string_view("1.0"));
  CHECK(std::string(ONEXEXPLORER_VERSION) == std::string("1.0"));
}
