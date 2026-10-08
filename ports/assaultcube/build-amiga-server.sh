#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${ASSAULTCUBE_SRC:-"$ROOT/build/game-ports/sources/assaultcube/source"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
ZLIB_ROOT=${ZLIB_ROOT:-"$HOME/AmigaChrome-dev/webkit-os32-040/m68k-amigaos"}
P="$STOVE/prefix"
CC="$P/bin/m68k-amigaos-gcc"
CXX="$P/bin/m68k-amigaos-g++"
AR="$P/bin/m68k-amigaos-ar"
RANLIB="$P/bin/m68k-amigaos-ranlib"
NM="$P/bin/m68k-amigaos-nm"

[ -f "$SRC/src/Makefile" ] || { echo "AssaultCube source not prepared: $SRC" >&2; exit 2; }
[ -f "$ZLIB_ROOT/include/zlib.h" ] && [ -f "$ZLIB_ROOT/lib/libz.a" ] || { echo "Amiga zlib not found: $ZLIB_ROOT" >&2; exit 3; }

# Apply only once to the disposable pinned source checkout.
if ! grep -q '__amigaos__' "$SRC/src/stream.cpp"; then
    (cd "$(dirname "$SRC")" && git apply "$HERE/patches/0001-amigaos-server-bootstrap.patch")
fi

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
