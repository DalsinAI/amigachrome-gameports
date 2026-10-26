#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
: "${OUTPUT:=terrain-coast-overlay-probe}"
s=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CXX" -std=c++17 -O1 -Wall -Wextra -Werror -m68040 -m68881 -fno-exceptions -fno-rtti "$s/terrain_mesh.cpp" "$s/terrain_coast_resources.cpp" "$s/terrain_coast.cpp" "$s/terrain_coast_overlay.cpp" "$s/terrain_coast_overlay_probe.cpp" -o "$OUTPUT"
