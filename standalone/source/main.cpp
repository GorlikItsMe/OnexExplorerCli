#include <onexexplorer/onexplorer.h>
#include <onexexplorer/version.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

auto main(int argc, char** argv) -> int {
  const std::unordered_map<std::string, onexexplorer::LanguageCode> languages{
      {"en", onexexplorer::LanguageCode::EN},
      {"de", onexexplorer::LanguageCode::DE},
      {"es", onexexplorer::LanguageCode::ES},
      {"fr", onexexplorer::LanguageCode::FR},
  };

  cxxopts::Options options(*argv,
                           "OnexExplorerCli - a tool for unpacking and repacking .NOS data files");

  std::string language;
  std::string name;

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("n,name", "Name to greet", cxxopts::value(name)->default_value("World"))
    ("l,lang", "Language code to use", cxxopts::value(language)->default_value("en"))
  ;
  // clang-format on

  auto result = options.parse(argc, argv);

  if (result["help"].as<bool>()) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  if (result["version"].as<bool>()) {
    std::cout << "OnexExplorerCli, version " << ONEXEXPLORER_VERSION << std::endl;
    return 0;
  }

  auto langIt = languages.find(language);
  if (langIt == languages.end()) {
    std::cerr << "unknown language code: " << language << std::endl;
    return 1;
  }

  onexexplorer::OnexExplorer app(name);
  std::cout << app.greet(langIt->second) << std::endl;

  return 0;
}
