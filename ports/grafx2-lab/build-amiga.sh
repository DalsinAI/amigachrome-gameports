#!/usr/bin/env bash
# GPL-2.0-only. Copyright (c) 2026 AmigaChrome Team
#
# Cradle's entry point (stove_build, target "."). STOVE is not used: the port
# pins its own compiler (OpenAmigaGCC 16.2.0-open3 release, checked by SHA-256)
# and OpenNDK from the same source release. OUT receives a lab-ready drawer:
# OUT/GrafX2/ (editor, data, Start-GrafX2), OUT/target-tests/ (guest checks)
# and OUT/colour-fixture.png.
[ -n "${BASH_VERSION:-}" ] || exec bash "$0" "$@"
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
: "${OUT:?OUT must be set}"
mkdir -p "$OUT"
WORK=${GRAFX2_BUILD_ROOT:-$OUT/.work}
mkdir -p "$WORK/downloads"

TAG=v16.2.0-open3
BIN=openamigagcc-16.2.0-open3-x86_64-linux.tar.xz
BIN_SHA=9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21
SRC=openamigagcc-16.2.0-open3-src.tar.xz
SRC_SHA=d8b7cf2101cf70df1cf719d199230260790420f43ec6e899f3d061a85ff3bcda
fetch() {  # file sha256
    [[ -f "$WORK/downloads/$1" ]] || curl -sSfL --retry 2 --max-time 900 \
        "https://github.com/DalsinAI/openamigagcc/releases/download/$TAG/$1" -o "$WORK/downloads/$1"
    printf '%s  %s\n' "$2" "$WORK/downloads/$1" | sha256sum --check --status \
        || { echo "hash mismatch: $1" >&2; exit 3; }
}
fetch "$BIN" "$BIN_SHA"
fetch "$SRC" "$SRC_SHA"
if [[ ! -x "$WORK/prefix/bin/m68k-amigaos-gcc" ]]; then
    mkdir -p "$WORK/prefix"
    tar --no-same-owner -xJf "$WORK/downloads/$BIN" -C "$WORK/prefix" --strip-components=1
fi
if [[ ! -d "$WORK/gccsrc/amiga-gcc" ]]; then
    mkdir -p "$WORK/gccsrc"
    tar --no-same-owner -xJf "$WORK/downloads/$SRC" -C "$WORK/gccsrc" --strip-components=1 \
        --wildcards '*/amiga-gcc/projects/openamigasdk/*'
fi
OPENSDK=$WORK/gccsrc/amiga-gcc/projects/openamigasdk
export OPENAMIGA_GCC=$WORK/prefix
export AMIGA_NDK=$OPENSDK/OpenNDK/Include_H
export OPENAMIGA_SPECS=$OPENSDK/Lib/m68k-soft-m68040.specs
export GRAFX2_WORK=$WORK/grafx2-work
rm -rf "$GRAFX2_WORK"
nice -n 19 python3 "$HERE/source/native-port/prepare.py"
nice -n 19 python3 "$HERE/source/native-port/build.py"
nice -n 19 python3 "$HERE/source/native-port/tests/build_target.py"

rm -rf "$OUT/GrafX2" "$OUT/target-tests"
mkdir -p "$OUT/GrafX2"
cp "$GRAFX2_WORK/build/GrafX2" "$GRAFX2_WORK/build/build-info.json" "$OUT/GrafX2/"
cp -r "$GRAFX2_WORK/grafx2/share/grafx2" "$OUT/GrafX2/data"
rm -rf "$OUT/GrafX2/data/scripts"
printf 'FailAt 21\nStack 524288\nGrafX2\n' > "$OUT/GrafX2/Start-GrafX2"
cp -r "$GRAFX2_WORK/target-tests" "$OUT/target-tests"
python3 "$HERE/source/native-port/tests/make_fixture.py" "$OUT/colour-fixture.png"
rm -rf "$WORK"
(cd "$OUT" && find . -type f | sort | xargs sha256sum) > "$OUT/SHA256SUMS"
cat "$OUT/SHA256SUMS" | head -40
