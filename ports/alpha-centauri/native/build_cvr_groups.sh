#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 metadata|raster|control OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "$1" in
 metadata) set -- "$src/cvr_model.cpp" "$src/cvr_groups_probe.cpp" -o "$2";;
 raster) set -- "$src/cvr_model.cpp" "$src/cvr_fixed_transform.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_group_raster_probe.cpp" -lm -o "$2";;
 control) set -- "$src/cvr_fixed_transform.cpp" "$src/cvr_groups_fixed_control.cpp" -o "$2";;
 *) exit 2;;
esac
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@"
