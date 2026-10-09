#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${ASSAULTCUBE_SRC:-"$ROOT/build/game-ports/sources/assaultcube/source"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
ZLIB_ROOT=${ZLIB_ROOT:-"$HOME/AmigaChrome-dev/webkit-os32-040/m68k-amigaos"}
P="$STOVE/prefix"
CXX="$P/bin/m68k-amigaos-g++"
[ -x "$P/bin/sdl2-config" ] || { echo "OpenGPU SDK missing from stove" >&2; exit 2; }
[ -f "$ZLIB_ROOT/lib/libz.a" ] || { echo "Amiga zlib missing" >&2; exit 3; }
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
