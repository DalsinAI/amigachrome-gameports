#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 leaf|world OUTPUT" >&2; exit 2; fi
mode=$1;output=$2;src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
set -- "$src/facility_upkeep.cpp"
case "$mode" in
leaf) set -- "$@" "$src/facility_upkeep_probe.cpp" ;;
world) for unit in rules_text original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_facility_upkeep world_facility_upkeep_probe; do set -- "$@" "$src/$unit.cpp"; done ;;
*) exit 2 ;;
esac
mkdir -p -- "$(dirname -- "$output")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@" -o "$output"
