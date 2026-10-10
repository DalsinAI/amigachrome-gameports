#!/usr/bin/env python3
"""Fetch publicly available runtime datasets for AmigaChrome game ports.

Nothing is fetched implicitly. Network access requires --allow-network.
Verified datasets are written to build/game-ports/test-data-cache by default.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import tempfile
import time
import urllib.request
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def load_manifest(root: Path) -> dict:
    path = root / "gameports" / "runtime-data.json"
    return json.loads(path.read_text(encoding="utf-8"))


def select_datasets(manifest: dict, games: list[str], datasets: list[str]) -> list[tuple[str, dict]]:
    all_ds = manifest["datasets"]
    picked: dict[str, dict] = {}
    if datasets:
        for name in datasets:
            if name not in all_ds:
                raise SystemExit(f"unknown dataset: {name}")
            picked[name] = all_ds[name]
    if games:
        for name, spec in all_ds.items():
            if any(game in spec.get("games", []) for game in games):
                picked[name] = spec
    if not games and not datasets:
        picked = dict(all_ds)
    return sorted(picked.items())


def fetch_one(name: str, spec: dict, cache: Path, allow_network: bool) -> dict:
    cache.mkdir(parents=True, exist_ok=True)
    dest = cache / spec["filename"]
    expected = spec.get("sha256")

    if dest.is_file():
        actual = sha256(dest)
        if expected and actual.lower() != expected.lower():
            dest.unlink()
        else:
            return {
                "dataset": name,
                "state": "cached",
                "path": str(dest),
                "sha256": actual,
                "expectedSha256": expected,
                "source": None,
            }

    if not allow_network:
        return {
            "dataset": name,
            "state": "missing-network-disabled",
            "path": str(dest),
            "expectedSha256": expected,
        }

    errors: list[str] = []
    for url in spec.get("urls", []):
        tmp = None
        try:
            with tempfile.NamedTemporaryFile(prefix=dest.name + ".", delete=False, dir=cache) as fh:
                tmp = Path(fh.name)
            req = urllib.request.Request(url, headers={"User-Agent": "AmigaChrome-GamePorts/1.0"})
            with urllib.request.urlopen(req, timeout=120) as src, tmp.open("wb") as out:
                shutil.copyfileobj(src, out)
            actual = sha256(tmp)
            if expected and actual.lower() != expected.lower():
                errors.append(f"{url}: checksum mismatch {actual}")
                tmp.unlink(missing_ok=True)
                continue
            tmp.replace(dest)
            return {
                "dataset": name,
                "state": "downloaded",
                "path": str(dest),
                "sha256": actual,
                "expectedSha256": expected,
                "source": url,
            }
        except Exception as exc:
            errors.append(f"{url}: {exc}")
            if tmp is not None:
                tmp.unlink(missing_ok=True)

    return {
        "dataset": name,
        "state": "failed",
        "path": str(dest),
        "expectedSha256": expected,
        "errors": errors,
    }


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, required=True)
    ap.add_argument("--game", action="append", default=[], help="fetch datasets required by this game; repeatable")
    ap.add_argument("--dataset", action="append", default=[], help="fetch an exact dataset id; repeatable")
    ap.add_argument("--cache", type=Path)
    ap.add_argument("--allow-network", action="store_true")
    args = ap.parse_args(argv)

    root = args.root.resolve()
    cache = (args.cache or (root / "build" / "game-ports" / "test-data-cache")).resolve()
    manifest = load_manifest(root)
    selected = select_datasets(manifest, args.game, args.dataset)

    results = [fetch_one(name, spec, cache, args.allow_network) for name, spec in selected]
    report = {
        "schema": 1,
        "createdAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "cache": str(cache),
        "networkAllowed": args.allow_network,
        "results": results,
    }
    cache.mkdir(parents=True, exist_ok=True)
    (cache / "RUNTIME_DATA_FETCH.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(report, indent=2, sort_keys=True))

    bad = [r for r in results if r["state"] in {"failed", "missing-network-disabled"}]
    return 2 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
