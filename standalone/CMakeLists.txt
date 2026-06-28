cmake_minimum_required(VERSION 3.14...3.22)

project(OnexExplorerCli LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage("gh:CLIUtils/CLI11@2.4.2")

CPMAddPackage(NAME OnexExplorer SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(CURL REQUIRED)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(${PROJECT_NAME} ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 OUTPUT_NAME "OnexExplorerCli")

target_link_libraries(${PROJECT_NAME} OnexExplorer::OnexExplorer CLI11::CLI11 CURL::libcurl)
