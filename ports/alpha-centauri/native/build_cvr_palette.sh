#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 2 ] || exit 2
case "$1" in faction) entry=cvr_palette_probe.cpp;; global) entry=cvr_global_palette_probe.cpp;; *) exit 2;; esac
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$2")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$src/cvr_palette.cpp" "$src/$entry" -o "$2"
