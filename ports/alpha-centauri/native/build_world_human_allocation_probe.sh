#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/rules_text.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" "$src/prototype.cpp" "$src/movement.cpp" "$src/saved_speed.cpp" "$src/world_unit_stats.cpp" "$src/vehicle_stack.cpp" "$src/tile_yields.cpp" "$src/world_tile_yields.cpp" "$src/technology_effects.cpp" "$src/world_fresh_tile_yields.cpp" "$src/world_worker_radius.cpp" "$src/world_terraform_ecology.cpp" "$src/citizens_text.cpp" "$src/world_citizen_energy.cpp" "$src/world_unit_support.cpp" "$src/human_worker_allocation.cpp" "$src/world_human_allocation.cpp" "$src/world_human_allocation_probe.cpp" -o "$1"
