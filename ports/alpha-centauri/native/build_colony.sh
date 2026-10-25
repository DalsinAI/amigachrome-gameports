#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then
  echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT" >&2
  exit 2
fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
  -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
  "$src/rules_text.cpp" "$src/game_logic.cpp" "$src/prototype.cpp" \
  "$src/movement.cpp" "$src/move_budget.cpp" "$src/exploration.cpp" \
  "$src/colony.cpp" "$src/colony_scenario.cpp" "$src/exploration_window.cpp" -o "$1"
