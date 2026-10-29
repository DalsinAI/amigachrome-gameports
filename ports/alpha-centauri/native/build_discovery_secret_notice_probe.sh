#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
if [ "$#" -ne 1 ]; then echo 'Usage: CXX=m68k-amigaos-g++ script OUTPUT' >&2; exit 2; fi
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p -- "$(dirname -- "$1")"
"${CXX:-m68k-amigaos-g++}" -m68040 -m68881 -std=c++17 -O2 -Wall -Wextra -Werror \
 -fno-exceptions -fno-rtti -fno-builtin -fno-tree-loop-distribute-patterns \
"$src/rules_text.cpp" "$src/original_map.cpp" "$src/original_game.cpp" \
 "$src/original_faction.cpp" "$src/original_world.cpp" "$src/prototype.cpp" \
 "$src/custom_prototype_construction.cpp" "$src/existing_prototype_reuse.cpp" "$src/proposer_record_stages.cpp" \
 "$src/prototype_prefix_context.cpp" "$src/automatic_prototype_name.cpp" "$src/human_component_pruning.cpp" \
 "$src/inactive_prototype_allocation.cpp" "$src/retirement_history.cpp" "$src/prototype_retirement_queue.cpp" \
 "$src/unused_prototype_retirement.cpp" "$src/unreferenced_prototype_allocation.cpp" "$src/discovery_reactor_sequence.cpp" "$src/technology_effects.cpp" "$src/monument_effects.cpp" "$src/discovery_continuation.cpp" "$src/discovery_secret_notice_probe.cpp" -o "$1"
