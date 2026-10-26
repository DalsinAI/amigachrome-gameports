#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-world-masks-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/original_map.cpp" "$s/original_game.cpp" "$s/original_faction.cpp" "$s/original_world.cpp" "$s/terrain_world_masks.cpp" "$s/terrain_world_masks_probe.cpp" -o "$OUTPUT"
