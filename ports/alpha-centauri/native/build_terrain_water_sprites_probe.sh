#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O1 -Wall -Wextra -Werror -fno-exceptions -fno-rtti "$src/sprite_scale.cpp" "$src/sprite_pixels.cpp" "$src/sprite_draw.cpp" "$src/terrain_sea_late.cpp" "$src/terrain_water_sprites.cpp" "$src/terrain_water_sprites_probe.cpp" -o "$1"
