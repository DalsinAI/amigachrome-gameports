#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 -DALPHA_EMBEDDED_VIEW "$src/rules_text.cpp" "$src/technology_browser.cpp" "$src/technology_browser_window.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" "$src/prototype.cpp" "$src/movement.cpp" "$src/saved_speed.cpp" "$src/world_unit_stats.cpp" "$src/world_movement_price.cpp" "$src/cvr_model.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_infantry.cpp" "$src/pcx_image.cpp" "$src/original_ui_text.cpp" "$src/original_map_window.cpp" "$src/original_frontend.cpp" -lSDL2_ttf -lSDL2 -lm -o "$1"
