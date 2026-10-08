#!/usr/bin/env python3
"""Neverball release package verifier.

This is deliberately strict: it verifies package completeness and provenance.
It does not turn a package into RELEASE; runtime qualification remains mandatory.
"""
from __future__ import annotations
import argparse, hashlib, json, zipfile
from pathlib import Path

PIN = "a1ed09911dca262d80049c12a2824d683af494d6"
REQUIRED = (
    "Neverball",
    "Neverputt",
    "LICENSE.md",
    "OPENUP-RELEASE.txt",
    "data",
    "doc",
)

def sha256(path: Path) -> str:
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(1024*1024), b""):
            h.update(b)
    return h.hexdigest()

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("package", type=Path)
    ap.add_argument("--report", type=Path)
    a=ap.parse_args()
    root=a.package.resolve()
    failures=[]
    for name in REQUIRED:
        if not (root/name).exists():
            failures.append(f"missing {name}")
    note=root/"OPENUP-RELEASE.txt"
    if note.is_file() and PIN not in note.read_text(errors="replace"):
        failures.append("release note does not carry the pinned upstream commit")
    data=root/"data"
    if data.is_dir():
        sols=list(data.rglob("*.sol"))
        pk3=data/"data-1.6.0.pk3"
        pk3_sols=0
        if pk3.is_file():
            if sha256(pk3) != "b2eddfbe05443e36719541639cb6246c5dd81260bce67a053d0d2c0ad34bc58c":
                failures.append("official data-1.6.0.pk3 hash mismatch")
            else:
                with zipfile.ZipFile(pk3) as zf:
                    pk3_sols=sum(1 for n in zf.namelist() if n.lower().endswith(".sol"))
        if not sols and pk3_sols == 0:
            failures.append("no compiled .sol level assets found")
    legal=root/"doc"/"legal"
    if not legal.is_dir():
        failures.append("upstream third-party legal notices missing")
    files=sorted(p for p in root.rglob("*") if p.is_file())
    report={
        "schema":1,
        "port":"neverball",
        "upstreamCommit":PIN,
        "package":str(root),
        "fileCount":len(files),
        "bytes":sum(p.stat().st_size for p in files),
        "failures":failures,
        "packageGate":"PASS" if not failures else "FAIL",
        "release":False,
        "releaseReason":"Runtime qualification is a separate mandatory gate."
    }
    if a.report:
        a.report.parent.mkdir(parents=True, exist_ok=True)
        a.report.write_text(json.dumps(report,indent=2,sort_keys=True)+"\n")
    print(json.dumps(report,indent=2,sort_keys=True))
    return 0 if not failures else 1

if __name__=="__main__":
    raise SystemExit(main())
