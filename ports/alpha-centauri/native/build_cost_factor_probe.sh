#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" = 1 ] || { echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; }
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/rules_text.cpp" "$src/original_map.cpp" "$src/original_game.cpp" \
 "$src/original_faction.cpp" "$src/original_world.cpp" "$src/cost_factor.cpp" \
 "$src/world_cost_factor.cpp" "$src/cost_factor_probe.cpp" -o "$1"
