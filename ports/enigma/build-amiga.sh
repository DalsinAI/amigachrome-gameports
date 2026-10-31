#!/bin/sh
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec python3 "$HERE/build_open3.py" "$@"
