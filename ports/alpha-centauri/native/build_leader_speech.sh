#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" = 1 ] || exit 2
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$src/leader_speech.cpp" "$src/leader_speech_audition.cpp" -lSDL2 -o "$1"
