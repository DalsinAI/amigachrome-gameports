#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 2 ]; then
  echo "Usage: CXX=m68k-amigaos-g++ $0 control|probe OUTPUT" >&2
  exit 2
fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$2")"
case "$1" in
 control) set -- "$2" "$src/vehicle_stack_control.cpp" ;;
 probe) set -- "$2" "$src/vehicle_stack_probe.cpp" "$src/original_map.cpp" "$src/original_game.cpp" "$src/original_faction.cpp" "$src/original_world.cpp" ;;
 *) exit 2 ;;
esac
output=$1
shift
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
 "$src/vehicle_stack.cpp" "$@" -o "$output"
