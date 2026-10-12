#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
OUT=${1:-"$ROOT/build/openformula68k"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
P="$STOVE/prefix"
SYS="$P/m68k-amigaos"
OLD_STOVE=${OLD_STOVE:-"$HOME/AmigaChrome/stoves/os32"}
OLD_SYS="$OLD_STOVE/prefix/m68k-amigaos"
CC="$P/bin/m68k-amigaos-gcc"
AR="$P/bin/m68k-amigaos-ar"
VERSION=0.1.0
PACKAGE="OpenFormula68K-$VERSION.zip"

[ -x "$CC" ] || { echo "OpenFormula68K: missing compiler $CC" >&2; exit 2; }
[ -x "$AR" ] || { echo "OpenFormula68K: missing archiver $AR" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT/obj" "$OUT/package/OpenFormula68K" "$OUT/sdk-compat" "$OUT/work"

EXTRA_INC="$OUT/sdk-compat"
if [ ! -f "$SYS/include/devices/ahi.h" ]; then
    [ -f "$OLD_SYS/include/devices/ahi.h" ] || { echo "OpenFormula68K: AHI headers are missing" >&2; exit 3; }
    mkdir -p "$EXTRA_INC/devices"
    cp "$OLD_SYS/include/devices/ahi.h" "$EXTRA_INC/devices/"
fi
if [ ! -f "$SYS/include/libraries/lowlevel.h" ]; then
    mkdir -p "$EXTRA_INC/libraries" "$EXTRA_INC/proto" "$EXTRA_INC/inline" "$EXTRA_INC/clib"
    if [ -f "$OLD_SYS/include/libraries/lowlevel.h" ] && [ -f "$OLD_SYS/include/proto/lowlevel.h" ]; then
        cp "$OLD_SYS/include/libraries/lowlevel.h" "$EXTRA_INC/libraries/"
        cp "$OLD_SYS/include/proto/lowlevel.h" "$EXTRA_INC/proto/"
        [ ! -f "$OLD_SYS/include/inline/lowlevel.h" ] || cp "$OLD_SYS/include/inline/lowlevel.h" "$EXTRA_INC/inline/"
        [ ! -f "$OLD_SYS/include/clib/lowlevel_protos.h" ] || cp "$OLD_SYS/include/clib/lowlevel_protos.h" "$EXTRA_INC/clib/"
    else
        # The current GCC16 stove intentionally carries only the headers needed
        # by the open stack. ACGame's lowlevel.library pad fallback is optional;
        # OpenInput and the keyboard remain fully available. Supply a compile-
        # time no-device shim rather than making the whole RTG game depend on an
        # unrelated legacy SDK header.
        cat > "$EXTRA_INC/libraries/lowlevel.h" <<'LOWLEVEL'
#ifndef OPENFORMULA_LOWLEVEL_COMPAT_H
#define OPENFORMULA_LOWLEVEL_COMPAT_H
#include <exec/types.h>
#define JP_TYPE_MASK       0xf0000000UL
#define JP_TYPE_NOTAVAIL   0x00000000UL
#define JP_TYPE_MOUSE      0x10000000UL
#define JP_TYPE_JOYSTK     0x20000000UL
#define JP_TYPE_GAMECTLR   0x30000000UL
#define JP_TYPE_UNKNOWN    0xf0000000UL
#define JPF_JOY_UP         0x00000001UL
#define JPF_JOY_DOWN       0x00000002UL
#define JPF_JOY_LEFT       0x00000004UL
#define JPF_JOY_RIGHT      0x00000008UL
#define JPF_BUTTON_RED     0x00000010UL
#define JPF_BUTTON_BLUE    0x00000020UL
#define JPF_BUTTON_GREEN   0x00000040UL
#define JPF_BUTTON_YELLOW  0x00000080UL
#define JPF_BUTTON_PLAY    0x00000100UL
#define JPF_BUTTON_REVERSE 0x00000200UL
#define JPF_BUTTON_FORWARD 0x00000400UL
#endif
LOWLEVEL
        cat > "$EXTRA_INC/proto/lowlevel.h" <<'LOWLEVELPROTO'
#ifndef OPENFORMULA_PROTO_LOWLEVEL_COMPAT_H
#define OPENFORMULA_PROTO_LOWLEVEL_COMPAT_H
#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/lowlevel.h>
extern struct Library *LowLevelBase;
#define ReadJoyPort(unit) ((void)(unit), (ULONG)JP_TYPE_NOTAVAIL)
#endif
LOWLEVELPROTO
    fi
fi

CFLAGS="-std=gnu99 -O2 -g -m68040 -m68881 -fomit-frame-pointer -fno-delete-null-pointer-checks -noixemul -Wall -Wextra"
INCLUDES="-I$SYS/include -I$EXTRA_INC -I$ROOT/port-layer/acgame/include -I$HERE/src"

compile() {
    src=$1
    obj=$2
    echo "CC $src"
    "$CC" $CFLAGS $INCLUDES -c "$src" -o "$obj"
}

compile "$ROOT/port-layer/acgame/src/c2p_ref.c" "$OUT/obj/c2p_ref.o"
compile "$ROOT/port-layer/acgame/src/ham8.c" "$OUT/obj/ham8.o"
compile "$ROOT/port-layer/acgame/src/platform_aros_aga.c" "$OUT/obj/platform_aros_aga.o"
compile "$HERE/src/audio_ahi.c" "$OUT/obj/audio_ahi.o"
GAME_SOURCE="$OUT/work/openformula.c"
cat "$HERE/src/openformula.c.part1" \
    "$HERE/src/openformula.c.part2" \
    "$HERE/src/openformula.c.part3" \
    "$HERE/src/openformula.c.part4" > "$GAME_SOURCE"
test -s "$GAME_SOURCE"
grep -F 'OpenFormula 68K' "$GAME_SOURCE" >/dev/null
grep -F 'returned RC' "$GAME_SOURCE" >/dev/null
compile "$GAME_SOURCE" "$OUT/obj/openformula.o"

"$AR" rcs "$OUT/obj/libacgame.a" \
    "$OUT/obj/c2p_ref.o" "$OUT/obj/ham8.o" "$OUT/obj/platform_aros_aga.o"

"$CC" -noixemul -m68040 -m68881 \
    "$OUT/obj/openformula.o" "$OUT/obj/audio_ahi.o" "$OUT/obj/libacgame.a" \
    -lamiga -o "$OUT/package/OpenFormula68K/OpenFormula68K"

file "$OUT/package/OpenFormula68K/OpenFormula68K"

cat > "$OUT/package/OpenFormula68K/Launch" <<'AMIGA'
FailAt 21
Stack 600000
CD PROGDIR:
OpenFormula68K
AMIGA

cat > "$OUT/package/OpenFormula68K/Launch-NoAudio" <<'AMIGA'
FailAt 21
Stack 600000
CD PROGDIR:
OpenFormula68K NOAUDIO
AMIGA

cat > "$OUT/package/OpenFormula68K/README.txt" <<'TXT'
OpenFormula 68K 0.1.0
=======================

An original Formula / Grand Prix racing game for AmigaOS 3.x RTG systems.
No licensed teams, drivers, circuits, cars, logos or commercial game data are
used. Graphics and sound are generated by the program.

Requirements
------------

* AmigaOS 3.x
* 68040 plus FPU (AC090 reference target)
* RTG 8-bit chunky mode (OpenRTG/Picasso96/CyberGraphX compatible)
* AHI is optional
* openinput.library or lowlevel.library is optional; keyboard always works

Workbench
---------

Double-click Launch. Launch-NoAudio is useful for diagnostics.

Controls
--------

Cursor left/right      Steer
Cursor up or Fire 1    Accelerate
Cursor down or Fire 2  Brake
Return / Play          Start race
Escape                 Exit

Game
----

Twelve original open-wheel cars race three laps of Boing Ring. The first
vertical slice includes a start grid, countdown, lap and position timing,
AI cars, collisions, hills, sweeping bends, hairpins, chicanes, off-road
slowdown, procedural engine audio and a chequered-flag result screen.

Shell options
-------------

OpenFormula68K [LAPS=n] [SECONDS=n] [NOAUDIO] [AUTOSTART] [AUTO] [AGA]

The supported release path is RTG. AUTO may choose RTG or native AGA. AGA is
kept as a compatibility and performance comparison path.
TXT

cat > "$OUT/package/OpenFormula68K/BUILD-INFO.txt" <<EOF2
OpenFormula 68K $VERSION
Target: AmigaOS 3.x / RTG / AC090 / 68040 + FPU
Compiler: $("$CC" --version | head -1)
Video: ACGame indexed 8-bit backend, RTG WriteChunkyPixels, optional OpenGfx acceleration
Input: keyboard, OpenInput controller, lowlevel joystick fallback
Audio: generated AHI 44.1 kHz signed 16-bit stereo
Assets: none; all visuals and audio are generated
EOF2

(
    cd "$OUT/package/OpenFormula68K"
    sha256sum OpenFormula68K Launch Launch-NoAudio README.txt BUILD-INFO.txt > SHA256SUMS
)
(
    cd "$OUT/package"
    zip -qr "$OUT/$PACKAGE" OpenFormula68K
)
sha256sum "$OUT/$PACKAGE" > "$OUT/SHA256SUMS"
printf 'OPENFORMULA_PACKAGE=%s\n' "$OUT/$PACKAGE"
cat "$OUT/SHA256SUMS"
