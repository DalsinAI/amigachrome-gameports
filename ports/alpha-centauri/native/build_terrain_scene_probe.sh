#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-scene-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/original_map.cpp" "$s/original_game.cpp" "$s/original_faction.cpp" "$s/original_world.cpp" "$s/rules_text.cpp" "$s/vehicle_stack.cpp" "$s/terrain_observation.cpp" "$s/terrain_visibility.cpp" "$s/terrain_caller_order.cpp" "$s/cvr_terrain_context.cpp" "$s/terrain_lighting.cpp" "$s/terrain_mesh.cpp" "$s/terrain_dispatch.cpp" "$s/terrain_texture.cpp" "$s/terrain_ground_plate.cpp" "$s/terrain_world_masks.cpp" "$s/terrain_coast.cpp" "$s/terrain_coast_resources.cpp" "$s/terrain_coast_overlay.cpp" "$s/terrain_ocean_detail.cpp" "$s/terrain_water_texture.cpp" "$s/terrain_water_patch.cpp" "$s/terrain_keyed_quad.cpp" "$s/terrain_feature_layers.cpp" "$s/terrain_forest_quad.cpp" "$s/terrain_scene.cpp" "$s/terrain_scene_probe.cpp" -o "$OUTPUT"
