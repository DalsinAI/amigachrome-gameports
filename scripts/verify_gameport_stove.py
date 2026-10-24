#!/usr/bin/env python3
"""Fail closed unless a game-port build uses the approved compiler stove."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from build_sdl3_port import verify_stove


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stove", required=True, type=Path)
    parser.add_argument("--release", default="16.2.0-open3", choices=("16.2.0-open2", "16.2.0-open3"))
    args = parser.parse_args()
    try:
        receipt = verify_stove(args.stove.resolve(), args.release)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.exit(2, f"STOVE_GATE_FAILED: {exc}\n")
    print(f'{receipt["release"]} {receipt["archive_sha256"]}')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
