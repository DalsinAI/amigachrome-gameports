#!/bin/sh
set -eu
: "${CXX:=m68k-amigaos-g++}"
cd "$(dirname "$0")"
"$CXX" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns original_template.cpp original_template_probe.cpp -o "${1:-original-template-probe}"
