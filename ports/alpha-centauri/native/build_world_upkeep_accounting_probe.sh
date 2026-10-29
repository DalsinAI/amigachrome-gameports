#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
engine=${ALPHA_RESOLUTION_SOURCE_ROOT:-$src}
mkdir -p -- "$(dirname -- "$1")"
output=$1
set --
for file in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support upkeep_base_energy upkeep_world_energy worker_allocation world_worker_upkeep upkeep_energy_allocation world_energy_allocation upkeep_base_psych world_base_psych cost_factor world_cost_factor world_worker_resolution; do set -- "$@" "$engine/$file.cpp"; done
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 -I "$engine" "$@" "$src/world_upkeep_accounting.cpp" "$src/world_upkeep_accounting_probe.cpp" -o "$output"
