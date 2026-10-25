#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 probe|control OUTPUT" >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "$1" in
 probe) set -- "$src/cvr_fixed_transform.cpp" "$src/cvr_fixed_transform_probe.cpp" -o "$2";;
 control) set -- "$src/cvr_fixed_transform.cpp" "$src/cvr_lighting.cpp" "$src/cvr_raster.cpp" "$src/cvr_fixed_object_control.cpp" -lm -o "$2";;
 *) exit 2;;
esac
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$@"
