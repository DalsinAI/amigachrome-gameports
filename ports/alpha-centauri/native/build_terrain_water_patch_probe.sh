#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-water-patch-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/original_map.cpp" "$s/original_game.cpp" "$s/original_faction.cpp" "$s/original_world.cpp" "$s/terrain_ocean_detail.cpp" "$s/terrain_water_texture.cpp" "$s/terrain_water_patch.cpp" "$s/terrain_water_patch_probe.cpp" -o "$OUTPUT"
