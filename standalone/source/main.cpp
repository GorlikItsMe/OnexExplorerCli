#include <onexexplorer/version.h>

#include <cxxopts.hpp>
#include <iostream>

auto main(int argc, char** argv) -> int {
  cxxopts::Options options(*argv,
                           "OnexExplorerCli - a tool for unpacking and repacking .NOS data files");

  // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
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

  return 0;
}
