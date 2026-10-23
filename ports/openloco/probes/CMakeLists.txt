cmake_minimum_required(VERSION 3.22)
project(OpenLocoPortProbes LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT EXISTS "${OPENLOCO_SOURCE}/src/OpenLoco/src/S5/SawyerStream.cpp")
    message(FATAL_ERROR "OPENLOCO_SOURCE must name the recipe's patched source export")
endif()
if(NOT EXISTS "${FMT_SOURCE}/include/fmt/format.h")
    message(FATAL_ERROR "FMT_SOURCE must name the pinned fmt source")
endif()
add_executable(wire-probe wire.cpp
    "${OPENLOCO_SOURCE}/src/OpenLoco/src/S5/SawyerStream.cpp"
    "${OPENLOCO_SOURCE}/src/Core/src/MemoryStream.cpp")
target_include_directories(wire-probe PRIVATE
    "${OPENLOCO_SOURCE}/src/OpenLoco/include/OpenLoco"
    "${OPENLOCO_SOURCE}/src/Core/include"
    "${OPENLOCO_SOURCE}/src/Core/include/OpenLoco/Core"
    "${FMT_SOURCE}/include")
target_compile_definitions(wire-probe PRIVATE FMT_HEADER_ONLY=1)
add_library(core-probes STATIC
    "${OPENLOCO_SOURCE}/src/Core/src/Numerics.cpp"
    "${OPENLOCO_SOURCE}/src/Core/src/Prng.cpp")
target_include_directories(core-probes PRIVATE
    "${OPENLOCO_SOURCE}/src/Core/include"
    "${OPENLOCO_SOURCE}/src/Core/include/OpenLoco/Core")
add_custom_target(port-probes DEPENDS wire-probe core-probes)
enable_testing()
add_test(NAME sawyer-wire-format COMMAND wire-probe)
