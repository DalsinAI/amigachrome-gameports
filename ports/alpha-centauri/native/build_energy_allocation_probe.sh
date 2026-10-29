#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 leaf|world|proposed OUTPUT" >&2; exit 2; fi
mode=$1; output=$2
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
set -- "$src/rules_text.cpp" "$src/upkeep_energy_allocation.cpp"
case "$mode" in
leaf) set -- "$@" "$src/upkeep_energy_allocation_probe.cpp" ;;
world) for unit in original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_energy_allocation world_energy_allocation_probe; do set -- "$@" "$src/$unit.cpp"; done ;;
proposed) for unit in original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_energy_allocation proposed_energy_allocation_probe; do set -- "$@" "$src/$unit.cpp"; done ;;
*) exit 2 ;;
esac
mkdir -p -- "$(dirname -- "$output")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$output"
