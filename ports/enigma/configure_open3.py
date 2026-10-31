#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Compatibility entry point: configure Enigma with OXP, never Xerces."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_open3 import REQUIRED, inventory, main
if __name__ == '__main__':
    sys.argv.append('--configure-only')
    main()
