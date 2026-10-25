#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo 'Usage: CXX=m68k-amigaos-g++ build_cvr_source_unit_probe.sh unit|flat OUTPUT' >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "$1" in
 unit) set -- "$src/cvr_source_unit.cpp" "$src/cvr_palette.cpp" "$src/cvr_model.cpp" "$src/cvr_map_anchor.cpp" "$src/cvr_projection.cpp" "$src/cvr_fixed_transform.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_scene.cpp" "$src/cvr_scene_blit.cpp" "$src/pcx_image.cpp" "$src/cvr_source_unit_probe.cpp" -lm -o "$2";;
 flat) set -- "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" "$src/cvr_flat_context.cpp" "$src/cvr_flat_context_probe.cpp" -o "$2";;
 *) exit 2;;
esac
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@"
