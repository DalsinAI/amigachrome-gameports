#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${COMPOSITION_OUTPUT:=terrain-composition-probe}"
: "${MATERIAL_OUTPUT:=terrain-material-probe}"
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$source_dir/terrain_texture.cpp" "$source_dir/terrain_composition_probe.cpp" -o "$COMPOSITION_OUTPUT"
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$source_dir/terrain_material.cpp" "$source_dir/terrain_material_probe.cpp" -o "$MATERIAL_OUTPUT"
