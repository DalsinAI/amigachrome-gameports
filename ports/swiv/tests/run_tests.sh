#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OUT="${TMPDIR:-/tmp}/nativertg-test-$$"
trap 'rm -f "$OUT"' EXIT HUP INT TERM
cc -std=c99 -O2 -Wall -Wextra -Werror -I"$ROOT/include" \
  "$ROOT/src/nativertg.c" "$ROOT/tests/test_nativertg.c" -o "$OUT"
"$OUT"
