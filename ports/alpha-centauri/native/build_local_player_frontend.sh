#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Native qualification candidate; do not deploy before the documented gate.
set -eu
src=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ALPHA_LOCAL_PLAYER_CANDIDATE=1 exec sh "$src/build_original_frontend.sh" "$@"
