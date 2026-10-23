#!/usr/bin/env python3
"""Offline SDL3 engineering builds; never installs or stages a release."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
OPEN2 = "5ece099a6fb8923a35eacf420bfa5fced06546ac1822714c76a2ae52466c978c"
OPEN3 = "9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21"


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def run(args, **kwargs):
    return subprocess.check_output([str(x) for x in args], text=True, **kwargs).strip()


def verify_source(path, pin):
    if run(["git", "-C", path, "rev-parse", "HEAD"]) != pin:
        raise ValueError(f"source revision mismatch: {path}")
    if run(["git", "-C", path, "status", "--porcelain", "--untracked-files=all"]):
        raise ValueError(f"source must be pristine: {path}")


def verify_stove(stove, release="16.2.0-open3"):
    receipt = json.loads((stove / "provenance.json").read_text())
    expected = {"16.2.0-open2": OPEN2, "16.2.0-open3": OPEN3}[release]
    if receipt["release"] != release or receipt["archive_sha256"] != expected:
        raise ValueError(f"Use scripts/stage_open2_stove.py --release {release} to stage the approved archive")
    files = receipt["compiler_runtime_sha256"]
    if "bin/m68k-amigaos-g++" not in files or len(files) < 100:
        raise ValueError("Incomplete compiler/runtime receipt")
    for name, expected in files.items():
        path = (stove / "prefix" / name).resolve()
        if not path.is_relative_to((stove / "prefix").resolve()) or digest(path) != expected:
            raise ValueError(f"compiler/runtime changed: {name}")
    return receipt


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--manifest", required=True, type=Path)
    ap.add_argument("--stove", required=True, type=Path, help="stove root, not prefix")
    ap.add_argument("--sdl3-sdk", required=True, type=Path, help="OpenRTG SDL3 build output")
    ap.add_argument("--source", type=Path)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--configure-only", action="store_true")
    args = ap.parse_args()
    manifest = args.manifest.resolve()
    spec = json.loads(manifest.read_text())
    game, port = next(iter(spec["ports"].items()))
    stove = args.stove.resolve()
    sdk = args.sdl3_sdk.resolve()
    source = (args.source or ROOT / "build/game-ports/sources" / game).resolve()
    out = (args.out or ROOT / "build/os3-sdl3" / game).resolve()
    if out.exists():
        raise ValueError(f"Use a new output directory for each clean build: {out}")
    if args.jobs < 1:
        raise ValueError("jobs must be positive")
    if os.environ.get("AMIGA_STOVE") and Path(os.environ["AMIGA_STOVE"]).resolve() != stove:
        raise ValueError("AMIGA_STOVE conflicts with --stove")
    receipt = verify_stove(stove, spec.get("compilerRelease", "16.2.0-open3"))
    verify_source(source, port["source"]["commit"])
    for name in ("libSDL3.a", "SDL3.module", "include/SDL3/SDL.h"):
        if not (sdk / name).is_file():
            raise ValueError(f"Missing OpenRTG SDL3 SDK file: {sdk / name}")
    # SDK input hashes identify imported binaries; they do not qualify them.
    sdk_hashes = {str(p.relative_to(sdk)): digest(p) for p in sorted(sdk.rglob("*"))
                  if p.is_file() and (p.is_relative_to(sdk / "include") or
                                     p.name in {"libSDL3.a", "SDL3.module"})}
    deps = {}
    for name, dep in spec.get("buildDependencies", {}).items():
        path = ROOT / "build/game-ports/dependencies" / name
        verify_source(path, dep["commit"])
        deps[name] = str(path)
    codecs = None
    if port.get("lockedCodecs"):
        from port_dependencies import verify
        codecs = verify(stove, port["lockedCodecs"])
    out.mkdir(parents=True)
    work = out / "source"
    work.mkdir()
    # Export the pinned commit; never patch or reset a source cache.
    with tempfile.TemporaryFile() as archive:
        subprocess.run(["git", "-C", str(source), "archive", "HEAD"], stdout=archive, check=True)
        archive.seek(0)
        with tarfile.open(fileobj=archive) as tf:
            tf.extractall(work, filter="data")
    patches = sorted((manifest.parent / "patches").glob("*.patch"))
    for patch in patches:
        with patch.open() as content:
            subprocess.run(["patch", "--batch", "--forward", "-p1"], cwd=work, stdin=content, check=True)
    config = out / "sdl3-config"
    config.mkdir()
    # SDL3::SDL3 is the OpenGPU module CLIENT archive, not a private static SDL.
    (config / "SDL3Config.cmake").write_text(
        'set(SDL3_FOUND TRUE)\n'
        'add_library(SDL3::SDL3 STATIC IMPORTED GLOBAL)\n'
        'set_target_properties(SDL3::SDL3 PROPERTIES\n'
        f' IMPORTED_LOCATION "{sdk}/libSDL3.a"\n'
        f' INTERFACE_INCLUDE_DIRECTORIES "{sdk}/include"\n'
        ' INTERFACE_LINK_LIBRARIES "m;amiga")\n')
    # Keep __FILE__ diagnostics independent of the checkout and build paths.
    (config / "repro.cmake").write_text(
        f'add_compile_options("-ffile-prefix-map={ROOT}=/gameports"\n'
        f' "-ffile-prefix-map={out}=/port-build")\n')
    command = ["cmake", "-S", str(work), "-B", str(out / "cmake"),
               f"-DCMAKE_TOOLCHAIN_FILE={ROOT}/toolchain/amigaos3-opengpu.cmake",
               f"-DAMIGA_STOVE={stove}", f"-DSDL3_DIR={config}",
               f"-DCMAKE_PROJECT_INCLUDE={config}/repro.cmake",
               "-DCMAKE_BUILD_TYPE=Release", "-DFETCHCONTENT_FULLY_DISCONNECTED=ON",
               "-DFETCHCONTENT_UPDATES_DISCONNECTED=ON"] + [
                   x.replace("@PORT_DIR@", str(manifest.parent)) for x in port["cmake"]]
    command += [f"-DFETCHCONTENT_SOURCE_DIR_{name.upper()}={path}" for name, path in deps.items()]
    if codecs:
        command += [f"-DZLIB_ROOT={codecs}", f"-DPNG_PNG_INCLUDE_DIR={codecs}/include",
                    f"-DPNG_LIBRARY={codecs}/lib/libpng.a"]
    evidence = {"schema": 1, "port": game, "source": port["source"],
                "compiler_archive_sha256": receipt["archive_sha256"],
                "compiler_release": receipt["release"],
                "sdk_input_sha256": sdk_hashes,
                "patches": {p.name: digest(p) for p in patches},
                "recipe_files": {str(p.relative_to(manifest.parent)): digest(p)
                                 for p in sorted(manifest.parent.rglob("*")) if p.is_file()},
                "dependencies": spec.get("buildDependencies", {}), "configure": command,
                "codec_lock_sha256": digest(ROOT / "toolchain/port-dependencies.lock.json") if codecs else None,
                "build": ["cmake", "--build", str(out / "cmake"), "--target", port["target"],
                          "-j", str(args.jobs)], "state": "attempting", "runtime": "not tested"}
    evidence_file = out / "receipt.json"
    env = dict(os.environ, STOVE=str(stove), PKG_CONFIG_PATH="",
               PKG_CONFIG_LIBDIR=str(stove / "prefix/m68k-amigaos/lib/pkgconfig"))
    env["SOURCE_DATE_EPOCH"] = run(["git", "-C", source, "show", "-s", "--format=%ct", "HEAD"])
    evidence["source_date_epoch"] = env["SOURCE_DATE_EPOCH"]
    try:
        for step in ("configure",) if args.configure_only else ("configure", "build"):
            with (out / f"{step}.log").open("w") as log:
                subprocess.run(evidence[step], env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
        evidence["state"] = "configured" if args.configure_only else "compiled"
        if not args.configure_only:
            binary = out / "cmake" / port["binary"]
            evidence["binary_sha256"] = digest(binary)
            evidence["binary_file"] = run(["file", binary])
    except subprocess.CalledProcessError as exc:
        evidence["state"] = "blocked"
        evidence["failed_command"] = exc.cmd
        raise
    finally:
        evidence_file.write_text(json.dumps(evidence, indent=2) + "\n")
        print(f"Build evidence: {evidence_file}; runtime/release not qualified", flush=True)


if __name__ == "__main__":
    main()
