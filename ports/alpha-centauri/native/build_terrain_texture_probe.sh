#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-texture-probe}"
source_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 \
 -fno-exceptions -fno-rtti "$source_dir/terrain_texture.cpp" \
 "$source_dir/terrain_texture_probe.cpp" -o "$OUTPUT"
