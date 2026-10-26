#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-ground-plate-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/terrain_mesh.cpp" "$s/terrain_dispatch.cpp" "$s/terrain_texture.cpp" "$s/terrain_ground_plate.cpp" "$s/terrain_ground_plate_probe.cpp" -o "$OUTPUT"
