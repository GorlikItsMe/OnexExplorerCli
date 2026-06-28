cmake_minimum_required(VERSION 3.14...3.22)

project(OnexExplorerCli LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  GITHUB_REPOSITORY CLIUtils/CLI11
  GIT_TAG v2.4.2
  OPTIONS "CLI11_BUILD_TESTS OFF" "CLI11_BUILD_EXAMPLES OFF"
)

CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 OUTPUT_NAME "OnexExplorerCli")

target_link_libraries(
  ${PROJECT_NAME} OnexExplorer::OnexExplorer CLI11::CLI11 nlohmann_json::nlohmann_json
)
