#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)

SRC=${1:-"$ROOT/build/game-ports/sources/cdogs-sdl"}
BIN=${2:-"$ROOT/build/os3/cdogs/src/cdogs-sdl"}
OUT=${3:-"$ROOT/build/release/cdogs-sdl-engineering"}

test -f "$BIN"
rm -rf "$OUT"
mkdir -p "$OUT/C-Dogs/Legal"

cp "$BIN" "$OUT/C-Dogs/CDogs"

for dir in data dogfights missions graphics music sounds; do
    if [ -e "$SRC/$dir" ]; then
        cp -a "$SRC/$dir" "$OUT/C-Dogs/"
    fi
done

for doc in COPYING README.md; do
    if [ -f "$SRC/$doc" ]; then
        cp "$SRC/$doc" "$OUT/C-Dogs/Legal/"
    fi
done

if [ -d "$SRC/doc" ]; then
    cp -a "$SRC/doc/." "$OUT/C-Dogs/Legal/"
fi

cat > "$OUT/C-Dogs/NOT-FOR-PUBLICATION.txt" <<'EOF'
C-Dogs SDL AmigaChrome engineering package

This package contains upstream game data for runtime qualification.
It is NOT a public release artifact.

Before publication:
- rebuild with the approved fixed GCC stove;
- pass the AC090 runtime release gates;
- close the upstream asset-provenance review, including README_DATA.md;
- preserve all applicable code/data attribution and licence notices.
EOF

(
    cd "$OUT"
    find C-Dogs -type f -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS
)

file "$OUT/C-Dogs/CDogs"
sha256sum "$OUT/C-Dogs/CDogs"
du -sh "$OUT/C-Dogs"
