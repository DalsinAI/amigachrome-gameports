#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-forest-layers-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/terrain_mesh.cpp" "$s/terrain_keyed_quad.cpp" "$s/terrain_feature_layers.cpp" "$s/terrain_forest_quad.cpp" "$s/terrain_forest_layers_probe.cpp" -o "$OUTPUT"
