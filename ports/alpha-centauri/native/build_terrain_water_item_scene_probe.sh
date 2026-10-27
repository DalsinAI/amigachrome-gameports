#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
: "${CXX:=m68k-amigaos-g++}"
: "${OUT:=terrain-water-item-scene-probe}"
"$CXX" -m68040 -m68881 -std=c++17 -O1 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
  "$src/original_map.cpp" \
  "$src/original_game.cpp" \
  "$src/original_faction.cpp" \
  "$src/original_world.cpp" \
  "$src/rules_text.cpp" \
  "$src/vehicle_stack.cpp" \
  "$src/terrain_observation.cpp" \
  "$src/terrain_visibility.cpp" \
  "$src/terrain_caller_order.cpp" \
  "$src/cvr_terrain_context.cpp" \
  "$src/terrain_lighting.cpp" \
  "$src/terrain_mesh.cpp" \
  "$src/terrain_dispatch.cpp" \
  "$src/terrain_texture.cpp" \
  "$src/terrain_ground_plate.cpp" \
  "$src/terrain_world_masks.cpp" \
  "$src/terrain_coast.cpp" \
  "$src/terrain_coast_resources.cpp" \
  "$src/terrain_coast_overlay.cpp" \
  "$src/terrain_ocean_detail.cpp" \
  "$src/terrain_water_texture.cpp" \
  "$src/terrain_water_patch.cpp" \
  "$src/terrain_keyed_quad.cpp" \
  "$src/terrain_feature_layers.cpp" \
  "$src/terrain_forest_quad.cpp" \
  "$src/terrain_roads.cpp" \
  "$src/terrain_rivers.cpp" \
  "$src/terrain_line.cpp" \
  "$src/terrain_grid.cpp" \
  "$src/terrain_base_grid.cpp" \
  "$src/terrain_scene.cpp" \
  "$src/sprite_scale.cpp" \
  "$src/sprite_pixels.cpp" \
  "$src/sprite_draw.cpp" \
  "$src/terrain_sea_platform.cpp" \
  "$src/terrain_sea_scene.cpp" \
  "$src/terrain_sea_late.cpp" \
  "$src/terrain_water_sprites.cpp" \
  "$src/terrain_water_item_scene.cpp" \
  "$src/terrain_water_item_scene_probe.cpp" \
  -o "$OUT"
