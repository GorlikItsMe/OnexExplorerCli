cmake_minimum_required(VERSION 3.14...3.22)

project(OnexExplorerBench LANGUAGES CXX)

# Native C++ benchmark for OnexExplorerCli.
#
# Build:  cmake -Stest -Bbuild_test -DBUILD_BENCHMARK=ON
#         cmake --build build_test --target OnexExplorerBench -j$(nproc)
# Run:    ./build_test/benchmark/OnexExplorerBench
#
# Accepts optional .NOS file paths as arguments.  Default files are the same
# five archives used by the Python benchmark suite (benchmark/).

# --- Import tools ---
include(../../cmake/tools.cmake)

# ---- Dependencies ----

include(../../cmake/CPM.cmake)

CPMAddPackage(
  NAME zlib-ng
  GIT_TAG 2.2.2
  GITHUB_REPOSITORY zlib-ng/zlib-ng EXCLUDE_FROM_ALL YES
  OPTIONS "ZLIB_COMPAT ON" "ZLIB_ENABLE_TESTS OFF" "WITH_GTEST OFF"
)

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

CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(
  ${PROJECT_NAME} OnexExplorer::OnexExplorer zlibstatic lodepng
)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)

# disable -Werror to avoid pedantic issues with benchmark code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
