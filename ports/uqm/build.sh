#!/bin/sh
set -eu

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:?usage: build.sh /path/to/uqm-0.8.0-source [build-dir]}
BUILD=${2:-"$ROOT/build/uqm"}
: "${AROS_SYSROOT:?set AROS_SYSROOT to the AROS Developer sysroot}"
AROS_CC=${AROS_CC:-m68k-aros-gcc}
AROS_CXX=${AROS_CXX:-m68k-aros-g++}
AROS_AR=${AROS_AR:-m68k-aros-ar}
AROS_RANLIB=${AROS_RANLIB:-m68k-aros-ranlib}
CPU=${CPU:-68040}

rm -rf "$BUILD"
mkdir -p "$BUILD/work" "$BUILD/tools" "$BUILD/out"
cp -R "$SRC"/. "$BUILD/work"/

python3 "$ROOT/port-layer/uqm/apply_aros_bootstrap.py"   "$BUILD/work" --write-config "$BUILD/config.state"

if [ -f "$BUILD/work/build.sh" ]; then
  SC2="$BUILD/work"
elif [ -f "$BUILD/work/sc2/build.sh" ]; then
  SC2="$BUILD/work/sc2"
else
  echo "Could not locate UQM build.sh" >&2
  exit 1
fi

ln -sf "$(command -v "$AROS_CC")" "$BUILD/tools/gcc"
ln -sf "$(command -v "$AROS_CC")" "$BUILD/tools/cc"
ln -sf "$(command -v "$AROS_CXX")" "$BUILD/tools/g++"
ln -sf "$(command -v "$AROS_CXX")" "$BUILD/tools/c++"
command -v "$AROS_AR" >/dev/null 2>&1 && ln -sf "$(command -v "$AROS_AR")" "$BUILD/tools/ar" || true
command -v "$AROS_RANLIB" >/dev/null 2>&1 && ln -sf "$(command -v "$AROS_RANLIB")" "$BUILD/tools/ranlib" || true

(
  cd "$SC2"
  PATH="$BUILD/tools:$PATH"   BUILD_HOST=AROS   BUILD_HOST_ENDIAN=big   AROS_SDK="$AROS_SYSROOT"   BUILD_WORK="$BUILD/out"   CFLAGS="--sysroot=$AROS_SYSROOT -m$CPU -fno-delete-null-pointer-checks"   CXXFLAGS="--sysroot=$AROS_SYSROOT -m$CPU -fno-delete-null-pointer-checks"   LDFLAGS="--sysroot=$AROS_SYSROOT -m$CPU"   /bin/sh build.sh uqm
)

echo "UQM build completed under: $BUILD"
