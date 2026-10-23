#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
: "${STOVE:?set STOVE to an isolated stage_open2_stove.py --release 16.2.0-open3 output root}"
: "${SDL3_SDK:?set SDL3_SDK to the OpenRTG SDL3 build output}"
exec python3 "$ROOT/scripts/build_sdl3_port.py" --manifest "$HERE/source-demo.json" --stove "$STOVE" --sdl3-sdk "$SDL3_SDK" "$@"
