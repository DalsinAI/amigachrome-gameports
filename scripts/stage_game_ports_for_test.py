#!/usr/bin/env python3
"""Prepare and optionally install the AGA/040 game-port field-test payload."""
from __future__ import annotations

import argparse
import os
import hashlib
import json
import shutil
import subprocess
import sys
import time
import zipfile
from pathlib import Path


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def copy_file(source: Path, target: Path) -> bool:
    if not source.is_file():
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    return True


def copy_tree(source: Path, target: Path) -> bool:
    if not source.is_dir():
        return False
    shutil.copytree(source, target, dirs_exist_ok=True)
    return True


def stage(root: Path) -> Path:
    root = root.resolve()
    cpu_target = os.environ.get("AMIGACHROME_GAMEPORTS_MCPU", "68040").strip()
    out_name = "out" if cpu_target == "68040" else f"out-{cpu_target}"
    out = root / "build" / "game-ports" / out_name
    status_path = out / "GAME_BUILD_STATUS.json"
    if not status_path.is_file():
        raise FileNotFoundError(f"Game build status missing: {status_path}")

    status = json.loads(status_path.read_text(encoding="utf-8"))
    field = out / "field-test"
    shutil.rmtree(field, ignore_errors=True)
    field.mkdir(parents=True)

    manifest = {
        "schema": 1,
        "target": status.get("target", "m68k-aros/68040"),
        "canonicalTarget": status.get("canonicalTarget", "m68k-aros/68040"),
        "compatibilityBuild": bool(status.get("compatibilityBuild", False)),
        "createdAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "entries": {},
    }

    smoke_src = out / "acgame-aga-smoke"
    smoke_dst = field / "ACGame" / "acgame-aga-smoke"
    smoke_ready = copy_file(smoke_src, smoke_dst)
    manifest["entries"]["acgame-smoke"] = {
        "state": "ready" if smoke_ready else "missing",
        "command": "acgame-aga-smoke",
        "expected": "320x200 animated indexed AGA checker/gradient; press Escape to exit",
    }

    games = status.get("games", {})

    cdogs = games.get("cdogs-sdl", {})
    if cdogs.get("state") == "done":
        package = out / "cdogs-sdl-aga-bootstrap.zip"
        target = field / "C-Dogs"
        target.mkdir(parents=True, exist_ok=True)
        if package.is_file():
            with zipfile.ZipFile(package) as zf:
                zf.extractall(target)
            children = list(target.iterdir())
            if len(children) == 1 and children[0].is_dir():
                nested = children[0]
                for item in list(nested.iterdir()):
                    shutil.move(str(item), str(target / item.name))
                nested.rmdir()
        else:
            copy_file(out / "cdogs-sdl-aros", target / "cdogs-sdl")
        manifest["entries"]["cdogs-sdl"] = {
            "state": "ready",
            "command": "cdogs-sdl",
            "data": "bundled bootstrap data",
            "graphics": "SDL2 first-light",
        }
    else:
        manifest["entries"]["cdogs-sdl"] = {
            "state": "blocked",
            "reason": cdogs.get("error", "build did not complete"),
        }

    hurrican = games.get("hurrican", {})
    if hurrican.get("state") == "done":
        target = field / "Hurrican"
        copy_file(out / "hurrican-aros", target / "hurrican")
        copy_file(out / "HURRICAN_RUN.txt", target / "HURRICAN_RUN.txt")
        local_data = root / "build" / "game-ports" / "work" / "hurrican" / "Hurrican"
        copied = []
        for name in ("data", "lang"):
            if copy_tree(local_data / name, target / name):
                copied.append(name)
        for name in ("splashscreen.bmp", "readme.txt"):
            if copy_file(local_data / name, target / name):
                copied.append(name)
        manifest["entries"]["hurrican"] = {
            "state": "ready-local-test" if "data" in copied else "needs-data",
            "command": "hurrican",
            "graphics": "SDL2/GL1 bootstrap",
            "localAssets": copied,
            "redistribution": "local field-test staging only; do not publish asset payload",
        }
    else:
        manifest["entries"]["hurrican"] = {
            "state": "blocked",
            "reason": hurrican.get("error", "build did not complete"),
        }

    openomf = games.get("openomf", {})
    if openomf.get("state") == "done":
        target = field / "OpenOMF"
        copy_file(out / "openomf-aros-aga", target / "openomf")
        copy_file(out / "OPENOMF_RUN.txt", target / "OPENOMF_RUN.txt")
        engine_resources = root / "build" / "game-ports" / "work" / "openomf" / "resources"
        copy_tree(engine_resources, target / "resources")
        (target / "OMF2097-DATA-REQUIRED.txt").write_text(
            "OpenOMF requires the original One Must Fall 2097 freeware data.\n"
            "Use --fetch-public-data when staging to download the public OpenOMF asset archive automatically.\n",
            encoding="utf-8",
        )
        manifest["entries"]["openomf"] = {
            "state": "needs-user-data",
            "command": "openomf",
            "graphics": "native ACGame indexed AGA",
            "audio": "NULL first-light",
        }
    else:
        manifest["entries"]["openomf"] = {
            "state": "blocked",
            "reason": openomf.get("error", "build did not complete"),
        }

    uqm = games.get("uqm", {})
    if uqm.get("state") == "done":
        target = field / "UQM"
        copy_file(out / "uqm-aros", target / "uqm-aros")
        copy_file(out / "UQM_RUN.txt", target / "UQM_RUN.txt")
        (target / "UQM-CONTENT-REQUIRED.txt").write_text(
            "UQM 0.8 base content is available publicly from the project.\n"
            "Use --fetch-public-data when staging to fetch and install it automatically.\n",
            encoding="utf-8",
        )
        manifest["entries"]["uqm"] = {
            "state": "needs-user-data",
            "command": "uqm-aros -n <content-dir>",
            "graphics": "SDL2 first-light",
        }
    else:
        manifest["entries"]["uqm"] = {
            "state": "blocked",
            "reason": uqm.get("error", "build did not complete"),
        }

    openjazz_bin = out / "openjazz-aros"
    if openjazz_bin.is_file():
        target = field / "OpenJazz"
        copy_file(openjazz_bin, target / "OpenJazz")
        (target / "JAZZ-DATA-REQUIRED.txt").write_text(
            "OpenJazz requires Jazz Jackrabbit data.\n"
            "Use --fetch-public-data when staging to fetch the public shareware episode automatically, or supply your registered game data.\n",
            encoding="utf-8",
        )
        manifest["entries"]["openjazz"] = {
            "state": "needs-user-data",
            "command": "OpenJazz",
            "graphics": "SDL2 first-light",
            "sourceBuild": "manual cross-build probe; m68k AROS",
        }
    else:
        manifest["entries"]["openjazz"] = {"state": "missing", "reason": "openjazz-aros not built"}

    nx_bin = out / "nxengine-evo-aros"
    if nx_bin.is_file():
        target = field / "NXEngine"
        copy_file(nx_bin, target / "nxengine-evo")
        (target / "CAVE-STORY-DATA-REQUIRED.txt").write_text(
            "NXEngine-evo requires Cave Story freeware data.\n"
            "Use --fetch-public-data when staging to fetch the public NXEngine-compatible dataset automatically.\n",
            encoding="utf-8",
        )
        manifest["entries"]["nxengine-evo"] = {
            "state": "needs-user-data",
            "command": "nxengine-evo",
            "graphics": "SDL2 first-light",
            "sourceBuild": "manual cross-build probe; m68k AROS",
        }
    else:
        manifest["entries"]["nxengine-evo"] = {"state": "missing", "reason": "nxengine-evo-aros not built"}

    sdlpop_bin = out / "sdlpop-aros"
    if sdlpop_bin.is_file():
        target = field / "SDLPoP"
        copy_file(sdlpop_bin, target / "prince")
        (target / "PRINCE-DATA-REQUIRED.txt").write_text(
            "SDLPoP requires user-supplied Prince of Persia game data.\n"
            "Original game assets are not bundled.\n",
            encoding="utf-8",
        )
        manifest["entries"]["sdlpop"] = {
            "state": "needs-user-data",
            "command": "prince",
            "graphics": "SDL2 first-light",
            "sourceBuild": "m68k AROS cross-build with GCC builtin alloca shim",
        }
    else:
        manifest["entries"]["sdlpop"] = {"state": "missing", "reason": "sdlpop-aros not built"}

    chocolate_bin = out / "chocolate-doom-aros"
    if chocolate_bin.is_file():
        target = field / "ChocolateDoom"
        copy_file(chocolate_bin, target / "chocolate-doom")
        (target / "DOOM-IWAD-REQUIRED.txt").write_text(
            "Chocolate Doom requires a user-supplied compatible Doom IWAD.\n"
            "No proprietary IWAD data is bundled.\n",
            encoding="utf-8",
        )
        manifest["entries"]["chocolate-doom"] = {
            "state": "needs-user-data",
            "command": "chocolate-doom -iwad <IWAD>",
            "graphics": "SDL2 software framebuffer first-light",
            "sourceBuild": "m68k AROS cross-build with static SDL2_mixer CMake shim",
        }
    else:
        manifest["entries"]["chocolate-doom"] = {"state": "missing", "reason": "chocolate-doom-aros not built"}

    # Optional, local-only legal test data. Nothing here downloads data and the
    # cache lives under build/, so these assets are never made part of source control.
    test_cache = root / "build" / "game-ports" / "test-data-cache"
    provenance = {}

    freedoom = test_cache / "freedoom-0.13.0.zip"
    if freedoom.is_file() and (field / "ChocolateDoom").is_dir():
        with zipfile.ZipFile(freedoom) as zf:
            for member, outname in (
                ("freedoom-0.13.0/freedoom1.wad", "freedoom1.wad"),
                ("freedoom-0.13.0/COPYING.txt", "FREEDOOM-COPYING.txt"),
            ):
                with zf.open(member) as src, (field / "ChocolateDoom" / outname).open("wb") as dst:
                    shutil.copyfileobj(src, dst)
        manifest["entries"]["chocolate-doom"]["state"] = "ready-test-data"
        manifest["entries"]["chocolate-doom"]["command"] = "chocolate-doom -iwad freedoom1.wad"
        provenance["chocolate-doom"] = {
            "dataset": "Freedoom 0.13.0 Phase 1",
            "source": "https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip",
            "sha256": sha256(freedoom),
            "rights": "open-content test dataset; see bundled FREEDOOM-COPYING.txt",
        }

    jazz = test_cache / "jazz-shareware-1.1.zip"
    if jazz.is_file() and (field / "OpenJazz").is_dir():
        with zipfile.ZipFile(jazz) as zf:
            zf.extractall(field / "OpenJazz")
        manifest["entries"]["openjazz"]["state"] = "ready-shareware-test"
        provenance["openjazz"] = {
            "dataset": "Jazz Jackrabbit 1.1 Shareware",
            "source": "https://www.dosgamesarchive.com/file.php?id=2031",
            "sha256": sha256(jazz),
            "rights": "shareware test dataset; not the registered/full game",
        }

    cave = test_cache / "cavestory-nx-data.zip"
    if cave.is_file() and (field / "NXEngine").is_dir():
        with zipfile.ZipFile(cave) as zf:
            zf.extractall(field / "NXEngine")
        manifest["entries"]["nxengine-evo"]["state"] = "ready-freeware-test"
        provenance["nxengine-evo"] = {
            "dataset": "Cave Story / NXEngine extracted freeware data",
            "source": "https://www.cavestory.one/downloads/data.zip",
            "sha256": sha256(cave),
            "rights": "freeware game data for local compatibility testing",
        }

    # SDLPoP's pinned source tree already carries the runtime data expected by
    # the engine. Stage it locally; retain the two-level demo as a comparison
    # fixture without replacing the engine's normal data tree.
    sdlpop_target = field / "SDLPoP"
    sdlpop_source = root / "build" / "game-ports" / "sources" / "sdlpop"
    if sdlpop_target.is_dir() and (sdlpop_source / "data").is_dir():
        copy_tree(sdlpop_source / "data", sdlpop_target / "data")
        copy_file(sdlpop_source / "SDLPoP.ini", sdlpop_target / "SDLPoP.ini")
        demo = test_cache / "pop1-demo.zip"
        if demo.is_file():
            with zipfile.ZipFile(demo) as zf:
                zf.extractall(sdlpop_target / "reference-demo")
            demo_hash = sha256(demo)
        else:
            demo_hash = None
        manifest["entries"]["sdlpop"]["state"] = "ready-project-data-test"
        provenance["sdlpop"] = {
            "dataset": "SDLPoP project runtime data; PoP1 two-level demo retained as reference fixture",
            "source": "pinned SDLPoP source tree; demo https://www.popot.org/get_the_games/software/PoP1_demo.zip",
            "sha256Demo": demo_hash,
            "rights": "local test use; demo is explicitly the two-level demo, not a full commercial install",
        }

    omf_assets = test_cache / "openomf-assets.zip"
    omf_target = field / "OpenOMF"
    if omf_assets.is_file() and omf_target.is_dir():
        with zipfile.ZipFile(omf_assets) as zf:
            zf.extractall(omf_target)
        children = [p for p in omf_target.iterdir() if p.is_dir() and p.name.lower().startswith("openomf-assets")]
        if len(children) == 1:
            nested = children[0]
            for item in list(nested.iterdir()):
                target_path = omf_target / item.name
                if target_path.exists() and target_path.is_dir() and item.is_dir():
                    copy_tree(item, target_path)
                    shutil.rmtree(item)
                elif not target_path.exists():
                    shutil.move(str(item), str(target_path))
            if nested.exists() and not any(nested.iterdir()):
                nested.rmdir()
        manifest["entries"]["openomf"]["state"] = "ready-freeware-test"
        provenance["openomf"] = {
            "dataset": "One Must Fall 2097 freeware assets for OpenOMF",
            "source": "https://www.omf2097.com/pub/files/omf/openomf-assets.zip",
            "sha256": sha256(omf_assets),
            "rights": "public freeware dataset fetched from the URL documented by OpenOMF upstream",
        }

    uqm_pkg = test_cache / "uqm-0.8.0-content.uqm"
    uqm_target = field / "UQM"
    if uqm_pkg.is_file() and uqm_target.is_dir():
        packages = uqm_target / "content" / "packages"
        packages.mkdir(parents=True, exist_ok=True)
        copy_file(uqm_pkg, packages / uqm_pkg.name)
        (uqm_target / "UQM_RUN.txt").write_text(
            "Official UQM 0.8 content package staged for local field testing.\\n"
            "Launch with: uqm-aros -n content\\n",
            encoding="utf-8",
        )
        manifest["entries"]["uqm"]["state"] = "ready-official-content"
        manifest["entries"]["uqm"]["command"] = "uqm-aros -n content"
        provenance["uqm"] = {
            "dataset": "The Ur-Quan Masters 0.8.0 base content",
            "source": "https://sourceforge.net/projects/sc2/files/UQM/0.8/uqm-0.8.0-content.uqm/download",
            "sha256": sha256(uqm_pkg),
            "rights": "official UQM content package",
        }

    if provenance:
        (field / "TEST_DATA_PROVENANCE.json").write_text(
            json.dumps({
                "schema": 1,
                "purpose": "local AmigaChrome port compatibility testing only",
                "distribution": "not part of source control or release media",
                "datasets": provenance,
            }, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    plan = """AmigaChrome AGA Game Ports - First-Light Test Plan
====================================================

The manifest records whether this payload is the canonical 68040 build or a
lower-CPU compatibility build used only to exercise the port/AGA path.

Recommended guest location:
  DH1:AmigaChrome/GamePorts

1. ACGame smoke
   CD DH1:AmigaChrome/GamePorts/ACGame
   acgame-aga-smoke
   Expect: animated 320x200 indexed checker/gradient. Press Escape to exit.

2. C-Dogs
   CD DH1:AmigaChrome/GamePorts/C-Dogs
   cdogs-sdl
   Expect: SDL2 first-light startup. Record screen, keyboard/mouse input and exit behaviour.

3. Hurrican
   CD DH1:AmigaChrome/GamePorts/Hurrican
   hurrican
   Expect: SDL2/GL1 first-light startup. Local test assets are staged when available.

4. OpenOMF
   CD DH1:AmigaChrome/GamePorts/OpenOMF
   openomf
   Requires user-supplied One Must Fall 2097 data.
   Expect: native indexed AGA renderer first light; NULL audio is intentional.

5. The Ur-Quan Masters
   CD DH1:AmigaChrome/GamePorts/UQM
   uqm-aros -n <content-dir>
   Requires separately supplied UQM 0.8 content.

6. OpenJazz
   CD DH1:AmigaChrome/GamePorts/OpenJazz
   OpenJazz
   Requires user-supplied Jazz Jackrabbit data.
   Expect: SDL2 startup, menu/title screen, keyboard input and clean exit.

7. NXEngine-evo
   CD DH1:AmigaChrome/GamePorts/NXEngine
   nxengine-evo
   Requires user-supplied Cave Story data.
   Expect: SDL2 startup, first room/frame, keyboard input and clean exit.

8. SDLPoP
   CD DH1:AmigaChrome/GamePorts/SDLPoP
   prince
   Requires user-supplied Prince of Persia data.
   Expect: SDL2 startup, level-one animation/input and clean exit.

9. Chocolate Doom
   CD DH1:AmigaChrome/GamePorts/ChocolateDoom
   chocolate-doom -iwad <IWAD>
   Requires a user-supplied Doom IWAD.
   Expect: menu, E1M1/software rendering, keyboard input and audio status.

For every launch record:
  - did the process start
  - first visible frame/screen
  - input response
  - audio state
  - exit/crash behaviour
  - runtime/guest logs
"""
    (field / "FIRST_LIGHT_TEST_PLAN.txt").write_text(plan, encoding="utf-8")
    (field / "FIELD_TEST_MANIFEST.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    checksums = []
    for path in sorted(p for p in field.rglob("*") if p.is_file()):
        if path.name == "FIELD_TEST_SHA256SUMS":
            continue
        checksums.append(f"{sha256(path)}  {path.relative_to(field).as_posix()}")
    (field / "FIELD_TEST_SHA256SUMS").write_text(
        "\n".join(checksums) + "\n",
        encoding="utf-8",
    )
    return field


def install(root: Path, instance: Path, volume: str) -> Path:
    field = stage(root)
    instance = instance.expanduser().resolve()
    harddisks = instance / "devices" / "harddisks"
    if not harddisks.is_dir():
        raise FileNotFoundError(f"Not an AmigaChrome instance root: {instance}")
    volume_root = harddisks / volume
    if not volume_root.is_dir():
        raise FileNotFoundError(f"Instance volume does not exist: {volume_root}")

    target = volume_root / "AmigaChrome" / "GamePorts"
    if target.exists():
        stamp = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
        backup = target.with_name(f"GamePorts.previous-{stamp}")
        target.rename(backup)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(field, target)

    report = {
        "schema": 1,
        "instance": str(instance),
        "volume": volume,
        "target": str(target),
        "source": str(field),
        "installedAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    }
    (target / "INSTALL_REPORT.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return target



def fetch_public_data(root: Path) -> None:
    cmd = [
        sys.executable,
        str(root / "scripts" / "fetch_runtime_data.py"),
        "--root",
        str(root),
        "--allow-network",
    ]
    subprocess.run(cmd, check=True)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, required=True)
    ap.add_argument("--instance", type=Path)
    ap.add_argument("--volume", default="DH1")
    ap.add_argument("--fetch-public-data", action="store_true",
                    help="download publicly available/freeware/shareware/demo datasets before staging")
    args = ap.parse_args(argv)
    root = args.root.resolve()
    if args.fetch_public_data:
        fetch_public_data(root)
    if args.instance:
        print(install(root, args.instance, args.volume))
    else:
        print(stage(root))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

