#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-visibility-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/original_map.cpp" "$s/original_game.cpp" "$s/original_faction.cpp" "$s/original_world.cpp" "$s/rules_text.cpp" "$s/vehicle_stack.cpp" "$s/terrain_observation.cpp" "$s/terrain_visibility.cpp" "$s/terrain_visibility_probe.cpp" -o "$OUTPUT"
