#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Build the browser (WebAssembly) Alpha Centauri frontend into OUTDIR.
#
#   build_wasm.sh OUTDIR
#
# Needs em++ on PATH (emsdk; tested with emscripten 6.0.13). The SDL2 and
# SDL2_ttf emscripten ports are used; where emscripten cannot download them,
# run seed_ports.sh first. OUTDIR receives index.html, shell.js, alpha.js and
# alpha.wasm; serve it over http(s) (any static server) and open index.html.
#
# The program is the native title screen (original_frontend.cpp) with the
# embedded map window, play layer, panels and engine: the same source list as
# native/build_original_frontend.sh, compiled unchanged. The only additions are
# amiga/amiga_shim.cpp (intuition/graphics/cybergraphics/asl calls drawn into
# a canvas), the include directory that routes the Amiga headers to it, and
# compat/libstdcxx_compat.h (std::__gcd, missing from libc++).
# No original game file is built in or shipped: the page asks for them.
set -eu
if [ "$#" -ne 1 ]; then echo "Usage: $0 OUTDIR" >&2; exit 2; fi
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
src=$here/../native
out=$1
mkdir -p -- "$out"
set --
for unit in play_session ui_research ui_social ui_design ui_battle ui_diplowin ui_newgame ui_diplo; do
 set -- "$@" "$src/$unit.cpp"
done
set -- "$@" "$src"/engine/*.cpp
for stub in "$src"/engine/stubs/*.cpp; do [ -f "$stub" ] && set -- "$@" "$stub"; done
for unit in rules_text technology_browser technology_browser_window original_map original_game original_faction original_world prototype movement saved_speed world_unit_stats world_movement_price cvr_model cvr_lighting cvr_raster cvr_infantry cvr_palette cvr_vehicle map_overlays ui_panel ui_minimap ui_messages ui_basewin ui_mapview pcx_image original_ui_text original_map_window leader_speech audio_wav audio_mixer audio_events audio_music audio_service audio_world original_frontend startup_movie startup_movie_player; do
 set -- "$@" "$src/$unit.cpp"
done
set -- "$@" "$here/amiga/amiga_shim.cpp"
# Clang reports four things GCC does not (brace elision in the engine's
# tables, GCC-only warning pragmas, set-only globals, one indentation style
# in startup_movie.cpp); all else is -Werror.
# ASYNCIFY lets WaitPort/SDL_Delay/the file requester sleep inside the native
# blocking loops. INVOKE_RUN=0: shell.js calls main once files are in place.
${EMXX:-em++} -std=c++17 ${OPT:--O2} -Wall -Wextra -Werror \
 -Wno-missing-braces -Wno-unknown-warning-option -Wno-unused-but-set-global -Wno-misleading-indentation \
 -fno-exceptions -fno-rtti \
 -include "$here/compat/libstdcxx_compat.h" -I"$here/amiga/include" -I"$src/engine" -I"$src" \
 -DALPHA_EMBEDDED_VIEW -DALPHA_EMBEDDED_MOVIE \
 -sUSE_SDL=2 -sUSE_SDL_TTF=2 \
 -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=1048576 -sSTACK_SIZE=8MB \
 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=128MB \
 -sINVOKE_RUN=0 -sEXIT_RUNTIME=1 -sFORCE_FILESYSTEM=1 -sENVIRONMENT=web \
 -sEXPORTED_RUNTIME_METHODS=FS,callMain \
 "$@" -o "$out/alpha.js"
cp -- "$here/index.html" "$here/shell.js" "$out/"
ls -l -- "$out/alpha.js" "$out/alpha.wasm"
