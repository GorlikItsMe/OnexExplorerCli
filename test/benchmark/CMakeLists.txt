cmake_minimum_required(VERSION 3.14...3.22)

project(OnexExplorerBench LANGUAGES CXX)

# Native C++ benchmark for OnexExplorerCli.
#
# Build:  cmake -Stest -Bbuild_test -DBUILD_BENCHMARK=ON cmake --build build_test --target
# OnexExplorerBench -j$(nproc) Run:    ./build_test/benchmark/OnexExplorerBench
#
# Accepts optional .NOS file paths as arguments.  Default files are the same five archives used by
# the Python benchmark suite (benchmark/).

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

CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# fpng is used for measuring PNG encode time independently. (main CMakeLists.txt already creates the
# target when OnexExplorer is built from source)
if(NOT TARGET fpng)
  add_library(fpng STATIC ${OnexExplorer_SOURCE_DIR}/third_party/fpng/fpng.cpp)
  target_include_directories(fpng SYSTEM PUBLIC ${OnexExplorer_SOURCE_DIR}/third_party/fpng)
  target_compile_features(fpng PUBLIC cxx_std_17)
  set_target_properties(fpng PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_PROCESSOR MATCHES
                                                   "x86_64|AMD64|i[3-6]86"
  )
    target_compile_options(fpng PRIVATE -msse4.1 -mpclmul)
  endif()
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} OnexExplorer::OnexExplorer zlibstatic fpng)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)

# disable -Werror to avoid pedantic issues with benchmark code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
