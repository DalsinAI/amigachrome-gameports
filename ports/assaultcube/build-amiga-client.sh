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
CXX="$P/bin/m68k-amigaos-g++"
[ -x "$P/bin/sdl2-config" ] || { echo "OpenGPU SDK missing from stove" >&2; exit 2; }
[ -f "$ZLIB_ROOT/lib/libz.a" ] || { echo "Amiga zlib missing" >&2; exit 3; }
# Prepare every patch required by this target, including on repeat builds.
python3 "$HERE/prepare_source.py" "$SRC" --client

[ -f "$SRC/enet/.libs/libenet.a" ] || {
    echo "Build the Amiga ENet/server first: sh $HERE/build-amiga-server.sh" >&2; exit 2;
}

cd "$SRC/src"
SDK_CFLAGS="$(SDL2_RUNTIME=-mcrt=nix20 "$P/bin/sdl2-config" --cflags)"
SDK_LIBS="$(SDL2_RUNTIME=-mcrt=nix20 "$P/bin/sdl2-config" --libs)"
"$CXX" $SDK_CFLAGS -O2 -I../include -c amiga_runtime.cpp -o amiga_runtime.o
"$CXX" $SDK_CFLAGS -O2 -I../include -c amiga_audio_stub.cpp -o amiga_audio_stub.o
make client CXX="$CXX" PLATFORM=AmigaOS \
 CXXFLAGS="$SDK_CFLAGS -O2 -fomit-frame-pointer -Wall -fsigned-char" \
 CLIENT_INCLUDES="-I. -Ibot -I../enet/include -I../include -I$ZLIB_ROOT/include" \
 CLIENT_LIBS="amiga_runtime.o amiga_audio_stub.o -L../enet/.libs -lenet -lSDL2_image -lGL $SDK_LIBS -L$ZLIB_ROOT/lib -lz -lpthread -lsocket -lamiga -lm"
file ac_client
sha256sum ac_client
