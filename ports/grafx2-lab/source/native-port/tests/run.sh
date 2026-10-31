#!/bin/sh
set -eu
: "${AMIGA_NDK:?Set AMIGA_NDK to the NDK Include_H directory}"
: "${GRAFX2_SOURCE:?Set GRAFX2_SOURCE to the prepared source directory}"
task_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
test_out=${GRAFX2_TEST_OUTPUT:-"$task_dir/../.work/host-tests"}
mkdir -p "$test_out"
python3 "$task_dir/host_types.py" "$AMIGA_NDK" "$test_out/host-include"
# Used only by behavioral mocks. Real m68k builds always use the original NDK.
cc() { command "${HOST_CC:-cc}" -I "$test_out/host-include" "$@"; }
for name in native_screen native_input datatypes; do
  if [ "$name" = datatypes ]; then
    # NDK's legacy long-sized host types keep tag pointers intact on x86-64 mocks.
    extra=-U__STDC_VERSION__
  else
    extra=
  fi
  cc $extra -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I "$AMIGA_NDK" -I "$GRAFX2_SOURCE/src" "$task_dir/$name.c" -o "$test_out/$name"
  ASAN_OPTIONS=detect_leaks=0 "$test_out/$name" "$test_out"
done
# Compile only the directory routine reachable from the small regression test.
cc -D_NO_INLINE -D__USE_NEW_TIMEVAL__ -DUSE_AMIGA_NATIVE -D__amigaos__ \
  -DNOTTF=1 -DNORECOIL -D__no_pnglib__ -D__no_tifflib__ \
  -ffunction-sections -fdata-sections -I "$AMIGA_NDK" -I "$GRAFX2_SOURCE/src" \
  -c "$GRAFX2_SOURCE/src/io.c" -o "$test_out/native-io.o"
cc -D_NO_INLINE -D__USE_NEW_TIMEVAL__ -DUSE_AMIGA_NATIVE \
  -I "$AMIGA_NDK" -I "$GRAFX2_SOURCE/src" -Wl,--gc-sections \
  "$task_dir/native_cwd.c" "$test_out/native-io.o" -o "$test_out/native_cwd"
"$test_out/native_cwd"
# New OpenUp conversion worker and AmigaDOS path regression.
cc -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$AMIGA_NDK" -I "$GRAFX2_SOURCE/src" \
  "$task_dir/expand_host.c" "$GRAFX2_SOURCE/src/amigaexpand.c" -o "$test_out/expand_host"
ASAN_OPTIONS=detect_leaks=0 "$test_out/expand_host"
cc -D__USE_NEW_TIMEVAL__ -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I "$AMIGA_NDK" -I "$GRAFX2_SOURCE/src" \
  "$task_dir/native_realpath.c" -o "$test_out/native_realpath"
ASAN_OPTIONS=detect_leaks=0 "$test_out/native_realpath"
