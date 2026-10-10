#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/astromenace"}

# Apply the normal AmigaChrome integration first because the target-random
# patch adds its post-SDL reseed immediately after the diagnostic SDL marker.
python3 "$HERE/patch_amigaos3.py" "$SRC" "$HERE/audio_amigachrome.cpp"
python3 "$HERE/patch_amiga_random.py" "$SRC"

# The regular builder's patch pass is idempotent.  Keeping it as the final
# build authority avoids a second copy of the compiler/library/package logic.
exec sh "$HERE/build-amigaos3.sh" "$@"
