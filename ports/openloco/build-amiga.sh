#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
#
# Cradle's entry point (stove_build, target ports/openloco).
#
# A full OpenLoco build takes longer than one Cradle job, so this script
# keeps a work folder (OPENLOCO_WORK) and is called repeatedly:
#   - no build of this recipe yet: starts one in the background (nice 19)
#     and returns;
#   - a build running: prints its progress;
#   - the build finished: puts the game, SDL3.module and the receipt in OUT.
# The first build is a clean one (scripts/build_sdl3_port.py). Later recipes
# re-export the pinned source with the new patches into the same tree,
# changing only files whose contents differ, and build incrementally.
# STOVE from Cradle is not used: the recipe needs the staged open3 stove
# (provenance.json and the locked codecs), OPENLOCO_STOVE.
[ -n "${BASH_VERSION:-}" ] || exec bash "$0" "$@"
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
: "${OUT:?OUT must be set}"
mkdir -p "$OUT"
GP=${OPENLOCO_GAMEPORTS:-$HOME/Documents/ChatGPT/AmigaChrome/gameports}
STAGED=${OPENLOCO_STOVE:-$GP/build/team1-open3}
SDL3_SDK=${OPENLOCO_SDL3_SDK:-$GP/build/team1-sdl3-open3}
WORK=${OPENLOCO_WORK:-$HOME/AmigaChrome-dev/openloco-work}
JOBS=${OPENLOCO_JOBS:-$(( $(nproc) > 3 ? $(nproc) / 2 : 1 ))}
mkdir -p "$WORK"
KEY=$( (cd "$HERE" && find source.json patches compat -type f | LC_ALL=C sort | xargs sha256sum) | sha256sum | cut -c1-16)
echo "recipe $KEY, work $WORK, stove $STAGED, sdl3 $SDL3_SDK, jobs $JOBS"

# Install into the scratch lab (never an instance): its drives are host
# folders. Only while the lab is stopped, so its file system sees one state.
LAB=${OPENLOCO_LAB:-$HOME/AmigaChrome-dev/labs/t3-openloco}
DATA=${OPENLOCO_DATA:-/run/media/da1ek/WorkingSpace/AmigaChrome/mediastore/games/locomotion-pc-cd-es-2004/game-data}
install_lab() {
    local dh1=$LAB/devices/harddisks/DH1 dest
    [[ -d $dh1 ]] || { echo "no lab drive $dh1"; return 0; }
    case $LAB in */AmigaChromeInstances/*) echo "refusing an instance: $LAB"; return 1;; esac
    if pgrep -f -- "$LAB" >/dev/null; then echo "lab running: not installing"; return 0; fi
    dest=$dh1/Games/OpenLoco
    mkdir -p "$dest"
    cp "$OUT/OpenLoco" "$dest/OpenLoco"
    rm -rf "$dest/data"; cp -r "$OUT/data" "$dest/data"
    if [[ ! -f $dest/Locomotion/.installed ]]; then
        rm -rf "$dest/Locomotion"; mkdir -p "$dest/Locomotion"
        cp -r "$DATA"/. "$dest/Locomotion/"; touch "$dest/Locomotion/.installed"
    fi
    cp "$OUT/SDL3.module" "$dest/SDL3.module"
    # The lab gets OpenAL.module only when it has none of its own.
    local al=$LAB/devices/harddisks/DH0/Libs/OpenGPU/OpenAL.module
    if [[ ! -f $al ]]; then cp "$OUT/OpenAL.module" "$al"; echo "OpenAL.module put in the lab's LIBS:OpenGPU"; fi
    sha256sum "$al" "$OUT/OpenAL.module"
    echo "installed in $dest:"; ls "$dest"; ls "$dest/Locomotion" | head; find "$dest/Locomotion" -type f | wc -l
    echo "SDL3.module in the lab:"; find "$LAB/devices/harddisks" -maxdepth 4 -iname 'SDL3.module' -exec sha256sum {} + 2>/dev/null
    sha256sum "$OUT/SDL3.module"
}

lab_report() {
    local u=$LAB/devices/harddisks/DH1/Games/OpenLoco/User f
    [[ -d $u ]] || return 0
    echo "== lab User folder"; find "$u" -maxdepth 2 -printf '%TT %s %p\n' | sort | tail -n 25
    f=$(ls -t "$u"/logs/*.log 2>/dev/null | head -n 1) || true
    [[ -n $f ]] && { echo "== $f"; tail -n 80 "$f"; }
    return 0
}
lab_report

alive() { [[ -f "$WORK/build.pid" ]] && kill -0 "$(cat "$WORK/build.pid")" 2>/dev/null; }
if alive; then
    echo "BUILDING recipe $(cat "$WORK/building.key" 2>/dev/null) since $(stat -c %y "$WORK/build.pid")"
    for f in "$WORK/build.log" "$WORK/tree/configure.log" "$WORK/tree/build.log"; do
        [[ -f $f ]] && { echo "-- $f"; tail -n 4 "$f"; }
    done
    exit 0
fi
BIN=$WORK/tree/cmake/OpenLoco
if [[ -f "$WORK/done.key" && $(cat "$WORK/done.key") == "$KEY" && -f "$BIN" ]]; then
    cp "$BIN" "$OUT/OpenLoco"
    cp "$SDL3_SDK/SDL3.module" "$OUT/SDL3.module"
    cp "$WORK/OpenAL.module" "$OUT/OpenAL.module"
    cp "$WORK/tree/receipt.json" "$OUT/receipt.json" 2>/dev/null || true
    cp -r "$WORK/tree/source/data" "$OUT/data"
    tail -n 30 "$WORK/build.log" > "$OUT/build-tail.log"
    m68k=$STAGED/prefix/bin/m68k-amigaos-
    "${m68k}size" "$OUT/OpenLoco" > "$OUT/size.txt" 2>&1 || true
    ls -l "$OUT"; sha256sum "$OUT"/OpenLoco "$OUT"/SDL3.module; cat "$OUT/size.txt"
    echo "READY recipe $KEY"
    install_lab
    exit 0
fi
if [[ -f "$WORK/failed.key" && $(cat "$WORK/failed.key") == "$KEY" ]]; then
    echo "FAILED recipe $KEY"; grep -nE 'error|Error' "$WORK/build.log" | head -40; tail -n 60 "$WORK/build.log"
    grep -nE "error:|Error [0-9]" "$WORK"/tree/*.log | head -30
    rm -f "$WORK/failed.key"   # the next call tries again
    exit 1
fi

for f in "$STAGED/provenance.json" "$STAGED/prefix/bin/m68k-amigaos-g++" "$SDL3_SDK/libSDL3.a" "$SDL3_SDK/SDL3.module"; do
    [[ -e $f ]] || { echo "MISSING $f"; ls -la "$GP/build" | head -60; exit 1; }
done

# Copy this recipe out of Cradle's checkout: Cradle resets it between jobs.
rm -rf "$WORK/recipe.new"; mkdir -p "$WORK/recipe.new"
git -C "$ROOT" archive HEAD | tar x -C "$WORK/recipe.new"
# Unchanged files keep their times, so the incremental build stays small.
mkdir -p "$WORK/recipe"; rsync -rc --delete --exclude /build "$WORK/recipe.new/" "$WORK/recipe/"; rm -rf "$WORK/recipe.new"
echo "$KEY" > "$WORK/building.key"
cat > "$WORK/run.sh" <<RUN
set -euo pipefail
R=$WORK/recipe
cd "\$R"
if [[ ! -d $WORK/sources/openloco/.git ]]; then
    python3 scripts/game_port_prepare.py prepare --catalog ports/openloco/source.json --game openloco --allow-network
    rm -rf $WORK/sources; mkdir -p $WORK/sources
    cp -a build/game-ports/sources/openloco $WORK/sources/openloco
    cp -a build/game-ports/dependencies $WORK/sources/dependencies
fi
rm -rf build/game-ports/sources/openloco build/game-ports/dependencies
mkdir -p build/game-ports/sources
ln -sfn $WORK/sources/openloco build/game-ports/sources/openloco
ln -sfn $WORK/sources/dependencies build/game-ports/dependencies
# OpenAL.module and its link library, from OpenRTG at the recipe's pin,
# with the GCC 16 stove (OpenRTG's own build script and defaults).
OPENRTG=\$(python3 -c "import json; print(json.load(open('ports/openloco/source.json'))['openrtg']['commit'])")
if [[ ! -f $WORK/openal/\$OPENRTG/libopenal.a ]]; then
    [[ -d $WORK/openrtg/.git ]] || git clone -q https://github.com/DalsinAI/openrtg.git $WORK/openrtg
    git -C $WORK/openrtg cat-file -e "\$OPENRTG^{commit}" 2>/dev/null || git -C $WORK/openrtg fetch -q origin
    git -C $WORK/openrtg checkout -q --detach "\$OPENRTG"
    rm -rf $WORK/openal/\$OPENRTG
    STOVE=${STOVE:-$HOME/AmigaChrome/stoves/os32-gcc16}/prefix sh $WORK/openrtg/library/modules/openal/build.sh $WORK/openal/\$OPENRTG module stub
fi
AL=$WORK/openal/\$OPENRTG
mkdir -p $WORK/openal-cmake
cat > $WORK/openal-cmake/OpenALConfig.cmake <<CFG
set(OpenAL_FOUND TRUE)
if(NOT TARGET OpenAL::OpenAL)
add_library(OpenAL::OpenAL STATIC IMPORTED GLOBAL)
set_target_properties(OpenAL::OpenAL PROPERTIES
 IMPORTED_LOCATION "\$AL/libopenal.a"
 INTERFACE_INCLUDE_DIRECTORIES "\$AL/include"
 INTERFACE_LINK_LIBRARIES "m;amiga")
endif()
CFG
export OPENAL_CMAKE_DIR=$WORK/openal-cmake
cp \$AL/OpenAL.module $WORK/OpenAL.module
python3 scripts/build_sdl3_port.py --manifest ports/openloco/source.json --stove $STAGED \
    --sdl3-sdk $SDL3_SDK --out $WORK/tree --jobs $JOBS --reuse-out
tail -n 40 $WORK/tree/build.log
echo $KEY > $WORK/done.key
RUN
rm -f "$WORK/done.key"
setsid nohup nice -n 19 bash -c "bash '$WORK/run.sh' || echo $KEY > '$WORK/failed.key'" > "$WORK/build.log" 2>&1 < /dev/null &
echo $! > "$WORK/build.pid"
echo "STARTED recipe $KEY (pid $(cat "$WORK/build.pid"))"
