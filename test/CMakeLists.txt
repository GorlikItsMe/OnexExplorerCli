cmake_minimum_required(VERSION 3.14...3.22)

project(OnexExplorerTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage("gh:doctest/doctest@2.5.2")
CPMAddPackage("gh:TheLartians/Format.cmake@1.8.3")

CPMAddPackage("gh:nlohmann/json@3.11.3")

CPMAddPackage(
  NAME zlib
  GIT_TAG v1.3.1
  GITHUB_REPOSITORY madler/zlib EXCLUDE_FROM_ALL YES
  OPTIONS "ZLIB_BUILD_EXAMPLES OFF"
)

find_package(CURL REQUIRED)

if(TEST_INSTALLED_VERSION)
  find_package(OnexExplorer REQUIRED)
else()
  CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(
  ${PROJECT_NAME} doctest::doctest OnexExplorer::OnexExplorer zlibstatic
  nlohmann_json::nlohmann_json CURL::libcurl
)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(OnexExplorer PRIVATE -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(OnexExplorer PRIVATE /W4 /WX)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add OnexExplorerTests ----

enable_testing()

# Note: doctest and similar testing frameworks can automatically configure CMake tests. For other
# testing frameworks add the tests target instead: add_test(NAME ${PROJECT_NAME} COMMAND
# ${PROJECT_NAME})

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(OnexExplorer PRIVATE -O0 -g --coverage)
  target_link_options(OnexExplorer PRIVATE --coverage)
  target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()
