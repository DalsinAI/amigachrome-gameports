#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=$1;shift
mkdir -p -- "$(dirname -- "$output")"
for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients game_logic colony cost_factor world_cost_factor world_population base_minerals world_base_minerals base_support world_base_support base_energy world_energy world_base_accounting worker_allocation world_worker_upkeep facility_production_snapshot human_upkeep_snapshot human_upkeep_transaction human_upkeep_transaction_probe; do
 set -- "$@" "$src/$unit.cpp"
done
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$output"
