#!/bin/sh
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=$1
shift
mkdir -p -- "$(dirname -- "$output")"
for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats vehicle_stack tile_yields world_technology_effects world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients base_minerals world_base_minerals base_support world_base_support base_energy world_energy world_base_accounting energy_allocation world_energy_allocation facility_upkeep world_facility_upkeep cost_factor world_cost_factor world_base_budget base_commerce world_base_commerce world_traded_budget world_base_budget_probe; do
 set -- "$@" "$src/$unit.cpp"
done
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -lm -o "$output"
