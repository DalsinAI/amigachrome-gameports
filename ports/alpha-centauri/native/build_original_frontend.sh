#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=$1
shift
mkdir -p -- "$(dirname -- "$output")"
# Use an extracted OpenUp SDK without copying it into the compiler sysroot.
# With no override, keep the installed-SDK build used on the native build host.
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
 for unit in move_budget vehicle_stack world_actions facility_availability world_facility_options production_retool world_production_retool local_production_session local_session_actions movement_snapshot production_snapshot local_actions_snapshot local_player tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support upkeep_base_energy upkeep_world_energy world_base_accounting upkeep_energy_allocation world_energy_allocation cost_factor world_cost_factor facility_upkeep world_facility_upkeep world_base_budget; do
  set -- "$@" "$src/$unit.cpp"
 done
fi
# Play mode: the engine (engine/*.cpp, engine/stubs/*.cpp), the play command
# layer and its panels. -pthread is required on m68k-amigaos for the engine's
# libstdc++ use (see build_engine.sh); it creates no thread.
for unit in play_session ui_research ui_social ui_design ui_battle ui_diplowin ui_newgame ui_diplo; do
 set -- "$@" "$src/$unit.cpp"
done
set -- "$@" "$src"/engine/*.cpp
for stub in "$src"/engine/stubs/*.cpp; do [ -f "$stub" ] && set -- "$@" "$stub"; done # none once every package has landed
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 $local_flags -pthread -I"$src/engine" -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 -DALPHA_EMBEDDED_VIEW -DALPHA_EMBEDDED_MOVIE "$src/rules_text.cpp" "$src/technology_browser.cpp" "$src/technology_browser_window.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" "$src/prototype.cpp" "$src/movement.cpp" "$src/saved_speed.cpp" "$src/world_unit_stats.cpp" "$src/world_movement_price.cpp" "$src/cvr_model.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_infantry.cpp" "$src/cvr_palette.cpp" "$src/cvr_vehicle.cpp" "$src/map_overlays.cpp" "$src/ui_panel.cpp" "$src/ui_minimap.cpp" "$src/ui_messages.cpp" "$src/ui_basewin.cpp" "$src/ui_mapview.cpp" "$src/pcx_image.cpp" "$src/original_ui_text.cpp" "$src/original_map_window.cpp" "$src/leader_speech.cpp" "$src/audio_wav.cpp" "$src/audio_mixer.cpp" "$src/audio_events.cpp" "$src/audio_music.cpp" "$src/audio_service.cpp" "$src/audio_world.cpp" "$src/original_frontend.cpp" "$src/startup_movie.cpp" "$src/startup_movie_player.cpp" "$@" -lSDL2_ttf -lSDL2 -lm -o "$output"
