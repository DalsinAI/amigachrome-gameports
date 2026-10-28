#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 leaf|world|upkeep OUTPUT" >&2; exit 2; fi
mode=$1; output=$2
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
set -- "$src/research_progress.cpp"
case "$mode" in
leaf) set -- "$@" "$src/research_cost_reference.cpp" "$src/research_progress_probe.cpp" ;;
world) for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_research world_research_probe; do set -- "$@" "$src/$unit.cpp"; done ;;
upkeep) for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields technology_effects world_fresh_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support upkeep_base_energy upkeep_world_energy worker_allocation world_worker_upkeep upkeep_energy_allocation world_energy_allocation upkeep_base_psych world_base_psych cost_factor world_cost_factor world_worker_resolution world_upkeep_accounting world_research world_upkeep_research world_upkeep_research_probe; do set -- "$@" "$src/$unit.cpp"; done ;;
*) exit 2 ;;
esac
mkdir -p -- "$(dirname -- "$output")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$output"
