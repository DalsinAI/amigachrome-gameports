#!/usr/bin/env python3
"""Apply the reviewed AssaultCube patches to a disposable pinned checkout, offline."""
from pathlib import Path
import argparse
import json
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def prepare(source, client=False, *, pin=None, patches=None):
    source = Path(source).resolve()
    repo = source.parent
    catalog = ROOT / "gameports/catalog.json"
    pin = pin or json.loads(catalog.read_text())["ports"]["assaultcube"]["source"]["commit"]
    def git(*args):
        return subprocess.run(
            ["git", "-C", str(repo), *args],
            capture_output=True, text=True, check=True,
        ).stdout.strip()
    if Path(git("rev-parse", "--show-toplevel")).resolve() != repo or source.name != "source":
        raise ValueError("Expected the pinned checkout's source/ directory")
    if git("rev-parse", "HEAD") != pin:
        raise ValueError("AssaultCube source revision differs from catalog pin")
    if patches is None:
        names = ["0001-amigaos-server-bootstrap.patch"]
        if client:
            names.append("0002-opengpu-sdk-client-first-light.patch")
        names += ["0003-amiga-stream-single-definition.patch", "0004-match-mode-buffer-array-delete.patch"]
        patches = [HERE / "patches" / name for name in names]
    def applicable(patch, reverse=False):
        args = ["git", "-C", str(repo), "apply", "--check"]
        if reverse:
            args.append("--reverse")
        return subprocess.run([*args, str(patch)], capture_output=True).returncode == 0

    for patch in patches:
        patch = Path(patch).resolve()
        if applicable(patch):
            git("apply", str(patch))
        elif not applicable(patch, reverse=True):
            raise ValueError(f"AssaultCube patch does not match source tree: {patch.name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--client", action="store_true")
    args = parser.parse_args()
    try:
        prepare(args.source, args.client)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        parser.exit(4, f"ASSAULTCUBE_SOURCE_GATE_FAILED: {exc}\n")


if __name__ == "__main__":
    main()
