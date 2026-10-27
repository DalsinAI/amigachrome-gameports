#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=$1
shift
mkdir -p -- "$(dirname -- "$output")"
# Match the frontend's optional extracted OpenUp SDK selection.
if [ -n "${OPENUP_SDK:-}" ]; then
 for header in SDL2/SDL.h SDL2/SDL_ttf.h; do
  if [ ! -f "$OPENUP_SDK/include/$header" ]; then
   echo "Missing OpenUp SDK header: $OPENUP_SDK/include/$header" >&2; exit 2
  fi
 done
 set -- "$@" -isystem "$OPENUP_SDK/include" "-L$OPENUP_SDK/lib"
fi
# Qualification candidate only. Ordinary builds retain the read-only entry path.
local_flags=""
if [ "${ALPHA_LOCAL_PLAYER_CANDIDATE:-0}" = 1 ]; then
 local_flags="-DALPHA_LOCAL_PLAYER_CANDIDATE"
 for unit in move_budget vehicle_stack world_actions facility_availability world_facility_options production_retool world_production_retool local_production_session local_session_actions movement_snapshot production_snapshot local_actions_snapshot local_player tile_yields world_technology_effects world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients base_minerals world_base_minerals base_support world_base_support base_energy world_energy world_base_accounting energy_allocation world_energy_allocation cost_factor world_cost_factor facility_upkeep world_facility_upkeep world_base_budget; do
  set -- "$@" "$src/$unit.cpp"
 done
fi
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 $local_flags -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/rules_text.cpp" "$src/technology_browser.cpp" "$src/technology_browser_window.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" "$src/prototype.cpp" "$src/movement.cpp" "$src/saved_speed.cpp" "$src/world_unit_stats.cpp" "$src/world_movement_price.cpp" "$src/cvr_model.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_infantry.cpp" "$src/pcx_image.cpp" "$src/original_ui_text.cpp" "$src/original_map_window.cpp" "$@" -lSDL2_ttf -lSDL2 -lm -o "$output"
