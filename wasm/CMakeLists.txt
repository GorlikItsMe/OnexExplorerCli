cmake_minimum_required(VERSION 3.14)

project(
  onex-explorer-wasm
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES"
)

CPMAddPackage(
  NAME zlib-ng
  GIT_TAG 2.2.2
  GITHUB_REPOSITORY zlib-ng/zlib-ng EXCLUDE_FROM_ALL YES
  OPTIONS "ZLIB_COMPAT ON" "ZLIB_ENABLE_TESTS OFF" "WITH_GTEST OFF"
)

CPMAddPackage(
  NAME nlohmann_json
  GIT_TAG v3.11.3
  GITHUB_REPOSITORY nlohmann/json
  OPTIONS "JSON_BuildTests OFF" "JSON_Install ON"
)

add_library(fpng STATIC ../third_party/fpng/fpng.cpp)
target_include_directories(fpng SYSTEM PUBLIC ../third_party/fpng)
target_compile_features(fpng PUBLIC cxx_std_17)
set_target_properties(fpng PROPERTIES POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE core_sources CONFIGURE_DEPENDS ../source/archive/*.cpp ../source/codecs/*.cpp
     ../source/onex.cpp
)
add_library(onex_core STATIC ${core_sources})
target_include_directories(onex_core PUBLIC ../include)
target_compile_options(onex_core PRIVATE -fno-exceptions)
target_link_libraries(onex_core PUBLIC fmt::fmt nlohmann_json::nlohmann_json zlibstatic fpng)

add_executable(onex_explorer_wasm cpp/bindings.cpp)
target_link_libraries(onex_explorer_wasm PRIVATE onex_core)

target_link_options(
  onex_explorer_wasm
  PRIVATE
  --bind
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createOnexModule
  -sALLOW_MEMORY_GROWTH=1
  -sFILESYSTEM=1
  -sWASM=1
  -sSINGLE_FILE=0
  -sNO_EXIT_RUNTIME=1
  -sEXPORTED_RUNTIME_METHODS=["FS","callMain"]
  --emit-tsd
  onex.d.ts
)
