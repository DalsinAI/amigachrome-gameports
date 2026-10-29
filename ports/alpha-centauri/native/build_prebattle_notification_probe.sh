#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
[ "$#" -eq 1 ] || exit 2
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns "$src/prebattle_notification.cpp" "$src/prebattle_notification_probe.cpp" -o "$1"
