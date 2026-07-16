cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Default 0.0.0 for local/dev builds. Release CI overrides via -DONEX_EXPLORER_CLI_VERSION=<tag>.
set(ONEX_EXPLORER_CLI_VERSION
    "0.0.0"
    CACHE STRING "Project version"
)

project(
  OnexExplorer
  VERSION ${ONEX_EXPLORER_CLI_VERSION}
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

CPMAddPackage(
  NAME zlib-ng
  GIT_TAG 2.2.2
  GITHUB_REPOSITORY zlib-ng/zlib-ng EXCLUDE_FROM_ALL YES
  OPTIONS
    "ZLIB_COMPAT ON"
    "ZLIB_ENABLE_TESTS OFF"
    "WITH_GTEST OFF"
)

find_package(CURL REQUIRED)

CPMAddPackage(
  NAME nlohmann_json
  GIT_TAG v3.11.3
  GITHUB_REPOSITORY nlohmann/json
  OPTIONS "JSON_BuildTests OFF" "JSON_Install ON"
)

# lodepng is a single-file library without a CMake target, so we fetch the source via CPM and build
# the target ourselves.
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

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

target_link_libraries(
  ${PROJECT_NAME} PRIVATE fmt::fmt CURL::libcurl nlohmann_json::nlohmann_json
                          $<BUILD_INTERFACE:zlibstatic> $<BUILD_INTERFACE:lodepng>
)

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
set(VERSION_HEADER_LOCATION "onex/version.h")

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "fmt 10.2.1" "nlohmann_json 3.11.3" "CURL"
)
