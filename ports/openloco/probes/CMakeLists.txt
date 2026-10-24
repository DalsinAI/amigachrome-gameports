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

add_executable(asset-wire asset_wire.cpp)
target_include_directories(asset-wire PRIVATE
    "${OPENLOCO_SOURCE}/src/OpenLoco/include"
    "${OPENLOCO_SOURCE}/src/OpenLoco/include/OpenLoco"
    "${OPENLOCO_SOURCE}/src/Core/include"
    "${OPENLOCO_SOURCE}/src/Engine/include"
    "${OPENLOCO_SOURCE}/src/Math/include"
    "${OPENLOCO_SOURCE}/src/Audio/include")
add_executable(object-wire object_wire.cpp
    "${OPENLOCO_SOURCE}/src/OpenLoco/src/S5/SawyerStream.cpp"
    "${OPENLOCO_SOURCE}/src/Core/src/MemoryStream.cpp")
target_include_directories(object-wire PRIVATE
    "${OPENLOCO_SOURCE}/src/OpenLoco/include"
    "${OPENLOCO_SOURCE}/src/OpenLoco/include/OpenLoco"
    "${OPENLOCO_SOURCE}/src/Core/include"
    "${OPENLOCO_SOURCE}/src/Core/include/OpenLoco/Core"
    "${OPENLOCO_SOURCE}/src/Engine/include"
    "${OPENLOCO_SOURCE}/src/Math/include"
    "${OPENLOCO_SOURCE}/src/Audio/include"
    "${FMT_SOURCE}/include"
    "${SFL_SOURCE}/include")
target_compile_definitions(object-wire PRIVATE FMT_HEADER_ONLY=1)
add_dependencies(port-probes asset-wire object-wire)
add_test(NAME graphics-css-wire-format COMMAND asset-wire)
add_test(NAME object-fixed-wire-format COMMAND object-wire)

add_executable(s5-wire s5_wire.cpp
    "${OPENLOCO_SOURCE}/src/OpenLoco/src/S5/SawyerStream.cpp"
    "${OPENLOCO_SOURCE}/src/Core/src/MemoryStream.cpp")
get_target_property(s5_probe_includes object-wire INCLUDE_DIRECTORIES)
target_include_directories(s5-wire PRIVATE ${s5_probe_includes})
target_compile_definitions(s5-wire PRIVATE FMT_HEADER_ONLY=1)
add_dependencies(port-probes s5-wire)
add_test(NAME s5-wire-boundaries COMMAND s5-wire)
add_executable(s5-state s5_state.cpp
    "${OPENLOCO_SOURCE}/src/OpenLoco/src/S5/SawyerStream.cpp"
    "${OPENLOCO_SOURCE}/src/Core/src/MemoryStream.cpp")
target_include_directories(s5-state PRIVATE ${s5_probe_includes})
target_compile_definitions(s5-state PRIVATE FMT_HEADER_ONLY=1)
add_dependencies(port-probes s5-state)
add_test(NAME s5-state-roundtrip COMMAND s5-state)
add_executable(widget-content widget_content.cpp)
target_include_directories(widget-content PRIVATE ${s5_probe_includes})
add_dependencies(port-probes widget-content)
add_test(NAME widget-content-bits COMMAND widget-content)
