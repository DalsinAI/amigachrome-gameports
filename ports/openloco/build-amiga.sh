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

alive() { [[ -f "$WORK/build.pid" ]] && kill -0 "$(cat "$WORK/build.pid")" 2>/dev/null; }
if alive; then
    echo "BUILDING recipe $(cat "$WORK/building.key" 2>/dev/null) since $(stat -c %y "$WORK/build.pid")"
    grep -E '^\[ *[0-9]+%\]' "$WORK/build.log" | tail -1 || true
    tail -n 15 "$WORK/build.log"
    exit 0
fi
BIN=$WORK/tree/cmake/OpenLoco
if [[ -f "$WORK/done.key" && $(cat "$WORK/done.key") == "$KEY" && -f "$BIN" ]]; then
    cp "$BIN" "$OUT/OpenLoco"
    cp "$SDL3_SDK/SDL3.module" "$OUT/SDL3.module"
    cp "$WORK/tree/receipt.json" "$OUT/receipt.json" 2>/dev/null || true
    tail -n 30 "$WORK/build.log" > "$OUT/build-tail.log"
    m68k=$STAGED/prefix/bin/m68k-amigaos-
    "${m68k}size" "$OUT/OpenLoco" > "$OUT/size.txt" 2>&1 || true
    ls -l "$OUT"; sha256sum "$OUT"/OpenLoco "$OUT"/SDL3.module; cat "$OUT/size.txt"
    echo "READY recipe $KEY"
    exit 0
fi
if [[ -f "$WORK/failed.key" && $(cat "$WORK/failed.key") == "$KEY" ]]; then
    echo "FAILED recipe $KEY"; grep -nE 'error|Error' "$WORK/build.log" | head -40; tail -n 60 "$WORK/build.log"
    rm -f "$WORK/failed.key"   # the next call tries again
    exit 1
fi

for f in "$STAGED/provenance.json" "$STAGED/prefix/bin/m68k-amigaos-g++" "$SDL3_SDK/libSDL3.a" "$SDL3_SDK/SDL3.module"; do
    [[ -e $f ]] || { echo "MISSING $f"; ls -la "$GP/build" | head -60; exit 1; }
done

# Copy this recipe out of Cradle's checkout: Cradle resets it between jobs.
rm -rf "$WORK/recipe.new"; mkdir -p "$WORK/recipe.new"
git -C "$ROOT" archive HEAD | tar x -C "$WORK/recipe.new"
rm -rf "$WORK/recipe"; mv "$WORK/recipe.new" "$WORK/recipe"
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
mkdir -p build/game-ports/sources
ln -sfn $WORK/sources/openloco build/game-ports/sources/openloco
ln -sfn $WORK/sources/dependencies build/game-ports/dependencies
if [[ ! -f $WORK/tree/cmake/CMakeCache.txt ]]; then
    rm -rf $WORK/tree
    python3 scripts/build_sdl3_port.py --manifest ports/openloco/source.json --stove $STAGED \
        --sdl3-sdk $SDL3_SDK --out $WORK/tree --jobs $JOBS
    cat $WORK/tree/configure.log $WORK/tree/build.log | tail -n 200
else
    rm -rf $WORK/fresh; mkdir -p $WORK/fresh
    git -C $WORK/sources/openloco archive HEAD | tar x -C $WORK/fresh
    for p in ports/openloco/patches/*.patch; do patch -s --batch --forward -p1 -d $WORK/fresh < "\$p"; done
    rsync -rc --delete $WORK/fresh/ $WORK/tree/source/
    STOVE=$STAGED SOURCE_DATE_EPOCH=\$(git -C $WORK/sources/openloco show -s --format=%ct HEAD) \
        cmake --build $WORK/tree/cmake --target OpenLoco -j $JOBS
fi
echo $KEY > $WORK/done.key
RUN
rm -f "$WORK/done.key"
setsid nohup nice -n 19 bash -c "bash '$WORK/run.sh' || echo $KEY > '$WORK/failed.key'" > "$WORK/build.log" 2>&1 < /dev/null &
echo $! > "$WORK/build.pid"
echo "STARTED recipe $KEY (pid $(cat "$WORK/build.pid"))"
echo "== lab layout"; ls -la "$HOME/AmigaChrome-dev/labs/t3-openloco" 2>&1 | head -30
ls -la "$GP/build" 2>&1 | grep -E 'team1|openloco' | head
