#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Build the shared game engine (engine/*.cpp, engine/stubs/*.cpp) together
# with one or more program sources into OUTPUT.
#
#   CXX=m68k-amigaos-g++ build_engine.sh OUTPUT MAIN.cpp [MORE.cpp...]
#
# CXXFLAGS replaces the default target flags (the Amiga 68040 set used by the
# other build scripts); test_engine.py passes host sanitizer or m68k-linux
# flags this way. Work packages add engine/<topic>.cpp files and delete their
# stub file; no edit here is needed.
set -eu
if [ "$#" -lt 2 ]; then
  echo "Usage: CXX=m68k-amigaos-g++ $0 OUTPUT MAIN.cpp [MORE.cpp...]" >&2
  exit 2
fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=$1
shift
mkdir -p -- "$(dirname -- "$out")"
default_flags="-m68040 -m68881 -O2 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns"
# shellcheck disable=SC2086
"${CXX:-m68k-amigaos-g++}" -std=c++17 -Wall -Wextra -Werror ${CXXFLAGS:-$default_flags} \
  -I"$src/engine" "$src"/engine/*.cpp "$src"/engine/stubs/*.cpp "$@" -o "$out"
