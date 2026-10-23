cmake_minimum_required(VERSION 3.18)
project(OpenJazzAssetValidation LANGUAGES C CXX)
if(NOT OPENJAZZ_SOURCE OR NOT EXISTS "${OPENJAZZ_SOURCE}/src/main.cpp")
  message(FATAL_ERROR "OPENJAZZ_SOURCE must name the prepared pinned source")
endif()
set(NETWORK OFF CACHE BOOL "" FORCE)
set(SCALE OFF CACHE BOOL "" FORCE)
set(SDL_VERSION 2 CACHE STRING "" FORCE)
set(PORTABLE ON CACHE BOOL "" FORCE)
add_subdirectory("${OPENJAZZ_SOURCE}" engine)
# Keep the production startup and loaders; replace only the executable entry point.
set_property(SOURCE "${OPENJAZZ_SOURCE}/src/main.cpp" DIRECTORY "${OPENJAZZ_SOURCE}"
  APPEND PROPERTY COMPILE_DEFINITIONS main=OpenJazzGameMain)
target_sources(OpenJazz PRIVATE "${CMAKE_CURRENT_LIST_DIR}/asset_probe.cpp")
