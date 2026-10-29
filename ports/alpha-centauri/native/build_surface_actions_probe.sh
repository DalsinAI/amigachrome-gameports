#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
test "$#" = 1
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=$1
mkdir -p "$(dirname -- "$out")"
set --
for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_movement_price move_budget vehicle_stack world_actions facility_availability world_facility_options production_retool world_production_retool local_production_session local_session_actions movement_snapshot production_snapshot local_actions_snapshot local_player surface_actions_probe; do set -- "$@" "$src/$unit.cpp"; done
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$out"
