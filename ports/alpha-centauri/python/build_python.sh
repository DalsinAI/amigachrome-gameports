#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Build the shared library the Python package alpha_smac loads with ctypes:
# the engine (native/engine/*.cpp), the play layer (play_session.cpp), the
# panel modules and the OriginalWorld view reader, plus the C API shim
# asmac.cpp. Same sources as native/test_play_session.py builds for
# play_control, without play_control.cpp. Host compiler, no original assets.
#
#   build_python.sh [OUTPUT]      default: alpha_smac/libasmac.so
#
# -Wno-format-truncation: at -O2 GCC flags snprintf truncation in native
# ui_newgame.cpp that the -O1 native test builds do not analyse; the
# truncation is intended there and the native sources stay unchanged.
# CXX (default g++) and CXXFLAGS (default -O2) can be overridden; -fPIC,
# -shared and -fvisibility=hidden are always added, so only the asmac_*
# functions of asmac.h are exported.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
native=$(CDPATH= cd -- "$here/../native" && pwd)
out=${1:-"$here/alpha_smac/libasmac.so"}
mkdir -p -- "$(dirname -- "$out")"
stubs=$(ls "$native"/engine/stubs/*.cpp 2>/dev/null || true)
extra=""
for n in play_session ui_panel ui_research ui_social ui_design ui_battle ui_diplowin ui_basewin \
         map_overlays original_map original_game original_faction original_world pcx_image ui_newgame ui_diplo \
         rules_text prototype movement saved_speed world_unit_stats world_movement_price \
         cvr_model cvr_lighting cvr_raster cvr_infantry cvr_palette cvr_vehicle ui_minimap ui_messages ui_mapview audio_events leader_speech; do
  if [ -f "$native/$n.cpp" ]; then extra="$extra $native/$n.cpp"; fi
done
tmp="$out.tmp.$$"
# shellcheck disable=SC2086
"${CXX:-g++}" -std=c++17 ${CXXFLAGS:--O2} -Wall -Wextra -Werror -Wno-format-truncation -Wno-misleading-indentation -fPIC -shared -fvisibility=hidden \
  -I"$native/engine" -I"$native" -I"$here" \
  "$native"/engine/*.cpp $stubs $extra "$here/asmac.cpp" "$here/asmac_view.cpp" -o "$tmp"
mv -f -- "$tmp" "$out"
echo "built $out"
