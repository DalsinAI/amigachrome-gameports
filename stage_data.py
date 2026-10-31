#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Stage Heroes II data into a built fheroes2 package, locally.

From the demo archive that scripts/fetch_runtime_data.py fetched and checked
(dataset homm2-demo), or from a folder holding the player's own game (its
DATA and MAPS folders, and ANIM and MUSIC when present). Only those folders
are copied, into the package's data/, maps/, anim/ and music/. Nothing staged
here is committed or published.

    stage_data.py --package OUT/fheroes2 --archive CACHE/h2demo.zip
    stage_data.py --package OUT/fheroes2 --game-dir /path/to/HEROES2
"""
import argparse
import hashlib
import shutil
import sys
import zipfile
from pathlib import Path

DEMO_SHA256 = "12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c"
FOLDERS = ("data", "maps", "anim", "music")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--package", type=Path, required=True)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument("--archive", type=Path)
    src.add_argument("--game-dir", type=Path)
    args = ap.parse_args()
    if not (args.package / "fheroes2").is_file():
        ap.error("--package must be the built package folder holding fheroes2")
    copied = 0
    if args.archive:
        digest = hashlib.sha256(args.archive.read_bytes()).hexdigest()
        if digest != DEMO_SHA256:
            sys.exit(f"{args.archive}: SHA-256 {digest} is not the pinned demo archive")
        with zipfile.ZipFile(args.archive) as z:
            for info in z.infolist():
                parts = Path(info.filename).parts
                if info.is_dir() or len(parts) != 2 or parts[0].lower() not in FOLDERS:
                    continue
                if parts[1] in ("..", ".") or "/" in parts[1] or "\\" in parts[1]:
                    continue
                dest = args.package / parts[0].lower() / parts[1].lower()
                dest.parent.mkdir(parents=True, exist_ok=True)
                with z.open(info) as fin, dest.open("wb") as fout:
                    shutil.copyfileobj(fin, fout)
                copied += 1
    else:
        for folder in args.game_dir.iterdir():
            if not folder.is_dir() or folder.name.lower() not in FOLDERS:
                continue
            for f in folder.iterdir():
                if f.is_file():
                    dest = args.package / folder.name.lower() / f.name.lower()
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(f, dest)
                    copied += 1
    if not (args.package / "data/heroes2.agg").is_file():
        sys.exit("No data/heroes2.agg was staged: this is not Heroes II data")
    print(f"staged {copied} files into {args.package}")


if __name__ == "__main__":
    main()
