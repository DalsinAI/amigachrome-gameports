#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 raw|world OUTPUT" >&2; exit 2; fi
case "$1" in raw) entry=research_pool_probe.cpp;; world) entry=research_pool_world_probe.cpp;; *) exit 2;; esac
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$2")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/rules_text.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" \
 "$src/prototype.cpp" "$src/movement.cpp" "$src/saved_speed.cpp" "$src/world_unit_stats.cpp" "$src/research_options.cpp" \
 "$src/research_pool.cpp" "$src/$entry" -o "$2"
