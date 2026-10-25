#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo 'Usage: CXX=m68k-amigaos-g++ build_cvr_map_anchor_probe.sh OUTPUT' >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti "$src/cvr_map_anchor.cpp" "$src/cvr_map_anchor_probe.cpp" -o "$1"
