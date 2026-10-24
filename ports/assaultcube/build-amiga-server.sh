#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${ASSAULTCUBE_SRC:-"$ROOT/build/game-ports/sources/assaultcube/source"}
: "${STOVE:?set STOVE to an isolated 16.2.0-open3 stove root}"
DEPS=$(python3 "$ROOT/scripts/port_dependencies.py" verify --stove "$STOVE" --require zlib)
python3 "$ROOT/scripts/verify_gameport_stove.py" --stove "$STOVE" >/dev/null
[ -z "${ZLIB_ROOT:-}" ] || [ "$ZLIB_ROOT" = "$DEPS" ] || {
    echo "ZLIB_ROOT must use the verified stove dependency directory: $DEPS" >&2; exit 3;
}
ZLIB_ROOT="$DEPS"
P="$STOVE/prefix"
CC="$P/bin/m68k-amigaos-gcc"
CXX="$P/bin/m68k-amigaos-g++"
AR="$P/bin/m68k-amigaos-ar"
RANLIB="$P/bin/m68k-amigaos-ranlib"
NM="$P/bin/m68k-amigaos-nm"

[ -f "$SRC/src/Makefile" ] || { echo "AssaultCube source not prepared: $SRC" >&2; exit 2; }
[ -f "$ZLIB_ROOT/include/zlib.h" ] && [ -f "$ZLIB_ROOT/lib/libz.a" ] || { echo "Amiga zlib not found: $ZLIB_ROOT" >&2; exit 3; }

# Prepare every patch required by this target, including on repeat builds.
python3 "$HERE/prepare_source.py" "$SRC"

cd "$SRC/enet"
make distclean >/dev/null 2>&1 || true
CC="$CC" AR="$AR" RANLIB="$RANLIB" NM="$NM" \
CFLAGS='-m68040 -m68881 -mcrt=nix20 -O2' \
./configure --host=m68k-amigaos --disable-shared --enable-static >/dev/null
make -j2 >/dev/null
"$RANLIB" .libs/libenet.a

cd "$SRC/src"
make server \
  CXX="$CXX" PLATFORM=AmigaOS \
  CXXFLAGS="-m68040 -m68881 -mcrt=nix20 -O2 -fomit-frame-pointer -Wall -fsigned-char -I$ZLIB_ROOT/include" \
  SERVER_LIBS="-L../enet/.libs -lenet -L$ZLIB_ROOT/lib -lz -lpthread -lsocket -lamiga"

echo "ASSAULTCUBE SERVER AMIGA PASS"
file ac_server
sha256sum ac_server
