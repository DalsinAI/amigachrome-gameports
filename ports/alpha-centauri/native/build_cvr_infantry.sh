#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then echo "Usage: CXX=m68k-amigaos-g++ $0 probe|control OUTPUT" >&2; exit 2; fi
case "$1" in probe|control) entry="cvr_infantry_$1.cpp";; *) exit 2;; esac
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$2")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/cvr_model.cpp" "$src/cvr_raster.cpp" "$src/cvr_lighting.cpp" "$src/cvr_infantry.cpp" "$src/$entry" -lm -o "$2"
