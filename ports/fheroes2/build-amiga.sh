#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
#
# Cradle's entry point (stove_build, target ports/fheroes2). STOVE is not used:
# the port pins its own SDK (build_sdk.sh). OUT gets the package ready for a
# lab: the game, OpenRTG's SDL2 modules and, with FHEROES2_DEMO=1 (default),
# the Heroes II demo data, which stays in the lab and is never published.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/../.." && pwd)
: "${OUT:?OUT must be set}"
mkdir -p "$OUT"
WORK=${FHEROES2_WORK:-$OUT/.work}
mkdir -p "$WORK"
PIN=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['commit'])" "$HERE/source.json")

[[ -f "$WORK/sdk/sdk.env" ]] || nice -n 19 bash "$HERE/build_sdk.sh" "$WORK/sdk"
if ! git -C "$WORK/src" cat-file -e "$PIN^{commit}" 2>/dev/null; then
    rm -rf "$WORK/src"; git init -q "$WORK/src"
    git -C "$WORK/src" fetch -q --depth 1 https://github.com/ihhub/fheroes2.git "$PIN"
fi
rm -rf "$WORK/game"
nice -n 19 bash "$HERE/build-amigaos3.sh" "$WORK/sdk" "$WORK/src" "$WORK/game"

PKG=$WORK/game/fheroes2
if [[ ${FHEROES2_DEMO:-1} == 1 ]]; then
    python3 "$ROOT/scripts/fetch_runtime_data.py" --root "$ROOT" --dataset homm2-demo \
        --cache "$WORK/data" --allow-network
    python3 "$HERE/stage_data.py" --package "$PKG" --archive "$WORK/data/h2demo.zip"
fi
rm -rf "$OUT/fheroes2"
cp -r "$PKG" "$OUT/fheroes2"
cp "$WORK/sdk/sdl2/SDL2.module" "$WORK/sdk/sdl2/SDL2_mixer.module" "$WORK/game/receipt.json" "$OUT/"
(cd "$OUT" && find fheroes2 -type f | sort) > "$OUT/FILES.txt"
rm -rf "$WORK"
cat "$OUT/FILES.txt"
