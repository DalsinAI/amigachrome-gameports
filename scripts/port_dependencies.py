#!/usr/bin/env python3
"""Stage/verify an explicit local codec inventory inside the selected stove.

No downloads or system search. Builds never use the source tree after staging.
Imported archives remain reused dependencies, not newly Open2-built libraries.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import sys
import tempfile

LOCK = Path(__file__).resolve().parents[1] / 'toolchain/port-dependencies.lock.json'
RELATIVE_DEST = Path('prefix/m68k-amigaos/port-deps')
GROUPS = {
    'zlib': {'lib/libz.a', 'include/zlib.h', 'include/zconf.h'},
    'png': {'lib/libpng.a', 'include/png.h', 'include/pngconf.h', 'include/pnglibconf.h'},
    'jpeg': {'lib/libjpeg.a', 'include/jconfig.h', 'include/jerror.h', 'include/jmorecfg.h', 'include/jpeglib.h'},
    'freetype': {'lib/libfreetype.a', 'include/freetype2/ft2build.h'},
}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def load_lock(path: Path = LOCK) -> dict:
    lock = json.loads(path.read_text())
    if lock.get('schema') != 1 or lock.get('status') != 'reused_not_rebuilt':
        raise ValueError('Unrecognised dependency lock format/status')
    if not isinstance(lock.get('files'), list) or not lock['files']:
        raise ValueError('Dependency lock is empty')
    seen = set()
    for item in lock['files']:
        rel = item['path']; p = PurePosixPath(rel); group = item['group']
        if p.is_absolute() or '..' in p.parts or str(p) != rel or rel in seen:
            raise ValueError('Unsafe or repeated dependency path: ' + rel)
        if group not in GROUPS:
            raise ValueError('Unknown dependency group: ' + group)
        permitted = rel in GROUPS[group] or (
            group == 'freetype' and rel.startswith('include/freetype2/') and rel.endswith('.h'))
        if not permitted:
            raise ValueError('File outside codec allow-list: ' + rel)
        sha = item['sha256']
        if len(sha) != 64 or any(c not in '0123456789abcdef' for c in sha):
            raise ValueError('Invalid dependency digest: ' + rel)
        seen.add(rel)
    for group, required in GROUPS.items():
        present = {item['path'] for item in lock['files'] if item['group'] == group}
        if not required.issubset(present):
            raise ValueError('Incomplete dependency group: ' + group)
    return lock


def contained_file(base: Path, relative: str) -> Path:
    path = base / relative
    # Do not let a symlink redirect a header/library to a workstation tree.
    for part in [path, *path.parents]:
        if part == base:
            break
        if part.is_symlink():
            raise ValueError('Symlink is not a staged input: ' + str(path))
    if not path.resolve().is_relative_to(base.resolve()) or not path.is_file():
        raise ValueError('Missing or external dependency: ' + str(path))
    return path


def destination(stove: Path) -> Path:
    stove = stove.resolve(strict=True)
    dest = stove / RELATIVE_DEST
    if not dest.resolve().is_relative_to(stove):
        raise ValueError('Dependency directory escapes the selected stove')
    if dest.is_symlink():
        raise ValueError('Dependency directory must not be a symlink')
    return dest


def stage(stove: Path, source: Path, lock_path: Path = LOCK) -> Path:
    lock = load_lock(lock_path); dest = destination(stove); source = source.resolve(strict=True)
    if dest.exists():
        raise ValueError('Refusing to overwrite staged dependencies: ' + str(dest))
    # Validate the complete inventory before creating the destination.
    inputs = []
    for item in lock['files']:
        src = contained_file(source, item['path'])
        if digest(src) != item['sha256']:
            raise ValueError('Source SHA-256 mismatch: ' + item['path'])
        inputs.append((item, src))
    dest.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.port-deps-', dir=dest.parent) as tmp:
        work = Path(tmp)
        for item, src in inputs:
            target = work / item['path']; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, target)
            if digest(target) != item['sha256']:
                raise ValueError('Copy SHA-256 mismatch: ' + item['path'])
        receipt = {'schema': 1, 'lock_sha256': digest(lock_path),
                   'status': 'reused_not_rebuilt', 'source': str(source), 'files': lock['files']}
        (work / 'manifest.json').write_text(json.dumps(receipt, indent=2) + '\n')
        if dest.exists():
            raise ValueError('Destination appeared during staging; refusing overwrite')
        work.rename(dest)
    return verify(stove, list(GROUPS), lock_path)


def verify(stove: Path, require: list[str], lock_path: Path = LOCK) -> Path:
    lock = load_lock(lock_path); dest = destination(stove)
    for group in require:
        if group not in GROUPS:
            raise ValueError('Unknown requested dependency: ' + group)
    manifest = contained_file(dest, 'manifest.json')
    receipt = json.loads(manifest.read_text())
    if receipt.get('lock_sha256') != digest(lock_path) or receipt.get('files') != lock['files']:
        raise ValueError('Staged manifest does not match the repository lock')
    expected = {'manifest.json'} | {item['path'] for item in lock['files']}
    for path in dest.rglob('*'):
        if path.is_symlink():
            raise ValueError('Symlink in staged dependencies: ' + str(path))
        if path.is_file() and str(path.relative_to(dest)) not in expected:
            raise ValueError('Unrecorded file in dependency include/library path: ' + str(path))
    # Check all staged files, not only libraries. Header drift is an ABI risk.
    for item in lock['files']:
        if digest(contained_file(dest, item['path'])) != item['sha256']:
            raise ValueError('Staged SHA-256 mismatch: ' + item['path'])
    return dest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('stage', 'verify'))
    parser.add_argument('--stove', type=Path, required=True)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--require', nargs='+', choices=tuple(GROUPS), default=list(GROUPS))
    args = parser.parse_args()
    try:
        if args.command == 'stage':
            if args.source is None:
                raise ValueError('stage requires an explicit --source; no fallback is searched')
            result = stage(args.stove, args.source)
        else:
            result = verify(args.stove, args.require)
    except (OSError, ValueError, KeyError, TypeError) as exc:
        parser.exit(3, 'DEPENDENCY_GATE_FAILED: ' + str(exc) + '\nStage the locked inputs inside this stove first.\n')
    print(result)
    return 0


if __name__ == '__main__':
    sys.exit(main())
