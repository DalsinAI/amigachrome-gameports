#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output=$1;shift
mkdir -p -- "$(dirname -- "$output")"
for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_movement_price move_budget vehicle_stack world_actions facility_availability world_facility_options production_retool world_production_retool local_production_session local_session_actions movement_snapshot production_snapshot local_actions_snapshot facility_production facility_production_stage facility_production_snapshot tile_yields world_technology_effects world_upkeep_tile_yields world_tile_yields world_worker_radius world_terraform_ecology world_stable_intake base_nutrients world_fresh_nutrients world_convoy_nutrients upkeep_base_minerals world_base_minerals base_support world_base_support cost_factor world_cost_factor fresh_facility_materials_probe; do
 set -- "$@" "$src/$unit.cpp"
done
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$output"
