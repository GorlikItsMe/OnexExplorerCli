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

CPMAddPackage(
  NAME zlib-ng
  GIT_TAG 2.2.2
  GITHUB_REPOSITORY zlib-ng/zlib-ng EXCLUDE_FROM_ALL YES
  OPTIONS
    "ZLIB_COMPAT ON"
    "ZLIB_ENABLE_TESTS OFF"
    "WITH_GTEST OFF"
)

# lodepng is fetched independently so the test builds under TEST_INSTALLED_VERSION=ON (where the
# root CMakeLists is not processed). CPM deduplicates when the root is also pulled in as a
# subproject.
CPMAddPackage(
  NAME lodepng
  GITHUB_REPOSITORY lvandeve/lodepng
  GIT_TAG ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a
)
if(lodepng_ADDED)
  add_library(lodepng STATIC ${lodepng_SOURCE_DIR}/lodepng.cpp)
  target_include_directories(lodepng SYSTEM PUBLIC ${lodepng_SOURCE_DIR})
  target_compile_features(lodepng PUBLIC cxx_std_17)
  set_target_properties(lodepng PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(TEST_INSTALLED_VERSION)
  find_package(OnexExplorer REQUIRED)
else()
  CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(
  ${PROJECT_NAME} doctest::doctest OnexExplorer::OnexExplorer zlibstatic lodepng
)
target_compile_definitions(
  ${PROJECT_NAME} PRIVATE ONEX_PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(OnexExplorer PUBLIC -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(OnexExplorer PUBLIC /W4 /WX)
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
  target_compile_options(OnexExplorer PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(OnexExplorer PUBLIC -fprofile-arcs -ftest-coverage)
endif()
