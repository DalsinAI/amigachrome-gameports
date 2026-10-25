#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 raw|world OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$2")"
case "$1" in
raw)
 "${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/technology_effects.cpp" "$src/technology_effects_probe.cpp" -o "$2";;
world)
 "${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/rules_text.cpp" "$src/original_map.cpp" "$src/original_game.cpp" \
 "$src/original_faction.cpp" "$src/original_world.cpp" "$src/technology_effects.cpp" \
 "$src/technology_effects_world_probe.cpp" -o "$2";;
*) exit 2;;
esac
