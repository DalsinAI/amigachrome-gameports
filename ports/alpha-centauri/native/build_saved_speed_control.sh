#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then
  echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2
  exit 2
fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
python3 "$src/test_saved_speed.py" --vectors-only "$1.vectors/saved_speed_vectors.inc"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
  -DALPHA_SPEED_VECTORS -I"$1.vectors" \
  "$src/saved_speed.cpp" "$src/saved_speed_control.cpp" -o "$1"
