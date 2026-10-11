#!/bin/sh
set -eu

ROOT=${1:-"$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"}
SRC=${2:-"$ROOT/build/game-ports/sources/astromenace"}
OUT=${3:-"$ROOT/build/releases/astromenace-ac090-0011"}
RELEASE_ID=${ASTROMENACE_RELEASE_ID:-0011}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}

rm -rf "$OUT"
mkdir -p "$OUT"

BUILD="$OUT/build"
ASTROMENACE_DIAGNOSTICS=0 \
ASTROMENACE_RELEASE_ID="$RELEASE_ID" \
STOVE="$STOVE" \
JOBS=${JOBS:-4} \
sh "$ROOT/ports/astromenace/build-amigaos3-runtime-fixed.sh" "$SRC" "$BUILD"

PKG="$BUILD/package/AstroMenace"
LAUNCHER="$PKG/AstroMenace"
BIN="$PKG/AstroMenace.bin"
VFS="$PKG/gamedata.vfs"
ARCHIVE_NAME="AstroMenace-AC090-GCC16-$RELEASE_ID.zip"
ARCHIVE="$OUT/$ARCHIVE_NAME"

[ -s "$LAUNCHER" ] || { echo "missing release launcher: $LAUNCHER" >&2; exit 9; }
[ -s "$BIN" ] || { echo "missing release game binary: $BIN" >&2; exit 10; }
[ -s "$VFS" ] || { echo "missing release VFS: $VFS" >&2; exit 11; }
[ ! -e "$PKG/gamedata" ] || { echo "release still carries duplicate raw gamedata" >&2; exit 12; }
[ -s "$PKG/LICENSE.md" ] || { echo "missing upstream licence" >&2; exit 13; }
[ -d "$PKG/licenses" ] || { echo "missing asset licence directory" >&2; exit 14; }
[ -s "$PKG/README-AmigaChrome.txt" ] || { echo "missing AmigaChrome release notes" >&2; exit 15; }
[ -s "$PKG/BUILD-INFO.txt" ] || { echo "missing build provenance" >&2; exit 16; }
[ -s "$PKG/SHA256SUMS" ] || { echo "missing in-package checksums" >&2; exit 17; }

file "$LAUNCHER" "$BIN"
strings "$LAUNCHER" | grep -F 'PROGDIR:AstroMenace.bin'

# Release packages must not contain the sentinel, guest stage-file names or the
# phase-by-phase asset trace used by the disposable qualification instance.
if strings "$BIN" | grep -E 'AUTOTEST_FIRST_MISSION|PROGDIR:stage-|AMLOAD:'; then
    echo "qualification-only strings leaked into release binary" >&2
    exit 18
fi
if find "$PKG" -maxdepth 1 -type f -name 'stage-*' -print -quit | grep -q .; then
    echo "qualification-only stage file leaked into release package" >&2
    exit 19
fi
strings "$BIN" | grep -F 'random engine reseeded'
strings "$BIN" | grep -F 'before joystick subsystem init'

python3 - "$VFS" <<'PY'
from pathlib import Path
import hashlib
import json
import struct
import sys

p = Path(sys.argv[1])
data = p.read_bytes()
sign, version, build, table = struct.unpack_from('<4s4sII', data, 0)
assert sign == b'VFS_', sign
assert version == b'v1.6', version
assert build == 250208, build
assert 16 < table < len(data), (table, len(data))
assert len(data) == 116020504, len(data)
print(json.dumps({
    'vfs': str(p),
    'size': len(data),
    'build': build,
    'tableOffset': table,
    'sha256': hashlib.sha256(data).hexdigest(),
}, indent=2, sort_keys=True))
PY

(
    cd "$BUILD/package"
    zip -qr "$ARCHIVE" AstroMenace
)

(
    cd "$OUT"
    sha256sum "$ARCHIVE_NAME" > SHA256SUMS
)

python3 - "$LAUNCHER" "$BIN" "$VFS" "$ARCHIVE" "$OUT/RELEASE-MANIFEST.json" "$RELEASE_ID" <<'PY'
from pathlib import Path
import hashlib
import json
import sys

launcher = Path(sys.argv[1])
binary = Path(sys.argv[2])
vfs = Path(sys.argv[3])
archive = Path(sys.argv[4])
destination = Path(sys.argv[5])
release_id = sys.argv[6]

def record(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    return {
        'name': path.name,
        'bytes': len(data),
        'sha256': hashlib.sha256(data).hexdigest(),
    }

manifest = {
    'schema': 'amigachrome-astromenace-release',
    'version': 2,
    'release': f'AC090-GCC16-{release_id}',
    'upstreamCommit': 'bbdb3ac5af2774c92b85c4d9b2a238f606911e66',
    'target': 'AmigaOS 3.x / AC090 / 68040 + FPU',
    'launcherStackBytes': 1048576,
    'files': [record(launcher), record(binary), record(vfs), record(archive)],
}
destination.write_text(
    json.dumps(manifest, indent=2, sort_keys=True) + '\n',
    encoding='utf-8',
)
print(destination.read_text(encoding='utf-8'))
PY

printf 'ASTROMENACE_RELEASE_ARCHIVE=%s\n' "$ARCHIVE"
cat "$OUT/SHA256SUMS"
