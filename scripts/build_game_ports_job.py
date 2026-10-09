#!/usr/bin/env python3
"""Kitchen job wrapper for the AGA/040 open-game port workstream."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path

ACTIONS = {
    "check": "Checking AGA/040 game-port pins and local contracts",
    "prepare": "Preparing exact upstream game-port sources (network allowed)",
    "build": "Building ACGame plus four AROS/68040 game lanes offline",
}
GUEST_PORT_COMMIT = "7a67157604a30895fb090f72c2603560e6d3a58e"
GUEST_REPOSITORY = "https://github.com/DalsinAI/amigachrome-guest.git"

CANONICAL_CPU_TARGET = "68040"
GAMEPORT_CPU_TARGET = os.environ.get(
    "AMIGACHROME_GAMEPORTS_MCPU", CANONICAL_CPU_TARGET
).strip()
if GAMEPORT_CPU_TARGET not in {"68020", "68030", "68040"}:
    raise RuntimeError(
        "AMIGACHROME_GAMEPORTS_MCPU must be one of 68020, 68030 or 68040"
    )
CPU_FLAG = f"-m{GAMEPORT_CPU_TARGET}"
# Address 0 is memory on an Amiga: without this flag GCC puts TRAP #7
# (Software Failure 80000027) where it proves a pointer null.
NULL_FLAG = "-fno-delete-null-pointer-checks"
GAME_TARGET = f"m68k-aros/{GAMEPORT_CPU_TARGET}"
OUTPUT_DIR_NAME = (
    "out" if GAMEPORT_CPU_TARGET == CANONICAL_CPU_TARGET
    else f"out-{GAMEPORT_CPU_TARGET}"
)


def paths(root: Path):
    work = root / "build" / "game-ports"
    return work, work / "kitchen-job.json", work / "kitchen-job.log"


def blank(root: Path) -> dict:
    work, _, _ = paths(root)
    return {
        "schema": 1,
        "state": "idle",
        "action": None,
        "message": "Ready.",
        "error": None,
        "pid": None,
        "startedAt": None,
        "finishedAt": None,
        "workRoot": str(work),
        "outputDir": str(work / OUTPUT_DIR_NAME),
    }


def read_json(path: Path, fallback: dict) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        return value if isinstance(value, dict) else fallback
    except (OSError, json.JSONDecodeError):
        return fallback


def write_json(path: Path, data: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(tmp, path)


def pid_alive(pid) -> bool:
    if not isinstance(pid, int) or pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def artifact_inventory(root: Path) -> list[dict]:
    out = root / "build" / "game-ports" / OUTPUT_DIR_NAME
    items = []
    for name in (
        f"libacgame-{GAMEPORT_CPU_TARGET}.a",
        f"c2p_ref-{GAMEPORT_CPU_TARGET}.o",
        f"ham8-{GAMEPORT_CPU_TARGET}.o",
        f"platform_aros_aga-{GAMEPORT_CPU_TARGET}.o",
        f"aga_smoke-{GAMEPORT_CPU_TARGET}.o",
        "acgame-aga-smoke",
        "cdogs-sdl-aros",
        "cdogs-sdl-aga-bootstrap.zip",
        "hurrican-aros",
        "HURRICAN_RUN.txt",
        "openomf-aros-aga",
        "OPENOMF_RUN.txt",
        "uqm-aros",
        "UQM_RUN.txt",
        "GAME_BUILD_STATUS.json",
        "BUILD_PROVENANCE.json",
        "SHA256SUMS",
    ):
        path = out / name
        if path.is_file():
            items.append({
                "name": name,
                "path": str(path),
                "size": path.stat().st_size,
                "sha256": sha256(path),
            })
    field = out / "field-test"
    for name in ("FIELD_TEST_MANIFEST.json", "FIELD_TEST_SHA256SUMS", "FIRST_LIGHT_TEST_PLAN.txt"):
        path = field / name
        if path.is_file():
            items.append({
                "name": f"field-test/{name}",
                "path": str(path),
                "size": path.stat().st_size,
                "sha256": sha256(path),
            })
    return items


def status(root: Path) -> dict:
    work, job_path, log_path = paths(root)
    data = read_json(job_path, blank(root))
    if data.get("state") == "running" and not pid_alive(data.get("pid")):
        data["state"] = "failed"
        data["error"] = data.get("error") or "Game-port job stopped without completing."
        data["message"] = data["error"]
        data["finishedAt"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        write_json(job_path, data)
    data["log"] = (
        "\n".join(log_path.read_text(encoding="utf-8", errors="replace").splitlines()[-500:])
        if log_path.is_file() else ""
    )
    data["artifacts"] = artifact_inventory(root)
    data["prepared"] = (work / "PREPARED.json").is_file()
    game_result = read_json(work / OUTPUT_DIR_NAME / "GAME_BUILD_STATUS.json", {})
    data["games"] = (
        game_result.get("games")
        if isinstance(game_result.get("games"), dict)
        else {}
    )
    return data


def start(root: Path, action: str) -> dict:
    if action not in ACTIONS:
        raise ValueError(f"Unknown game-port action: {action}")
    current = status(root)
    if current.get("state") == "running" and pid_alive(current.get("pid")):
        raise RuntimeError("A game-port Kitchen job is already running.")
    work, job_path, log_path = paths(root)
    work.mkdir(parents=True, exist_ok=True)
    data = blank(root)
    data.update({
        "state": "running",
        "action": action,
        "message": ACTIONS[action],
        "startedAt": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    })
    write_json(job_path, data)
    handle = log_path.open("wb")
    proc = subprocess.Popen(
        [sys.executable, str(Path(__file__).resolve()), "--root", str(root), "--run", action],
        cwd=root,
        stdout=handle,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    handle.close()
    data["pid"] = proc.pid
    write_json(job_path, data)
    return status(root)


def cancel(root: Path) -> dict:
    _, job_path, _ = paths(root)
    data = status(root)
    pid = data.get("pid")
    if pid_alive(pid):
        try:
            os.killpg(pid, signal.SIGTERM)
        except OSError:
            os.kill(pid, signal.SIGTERM)
    data.update(
        state="cancelled",
        message="Cancelled.",
        error="Cancelled.",
        finishedAt=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    )
    write_json(job_path, data)
    return status(root)


def run(
    cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None
) -> None:
    print("+ " + " ".join(cmd), flush=True)
    subprocess.run(cmd, cwd=cwd, env=env, check=True)


def ensure_port_layer(root: Path) -> Path:
    work = root / "build" / "game-ports"
    guest = work / "port-layer"
    if guest.is_dir() and (guest / ".git").is_dir():
        got = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=guest, text=True
        ).strip().lower()
        if got == GUEST_PORT_COMMIT:
            return guest
        import shutil
        shutil.rmtree(guest)
    run(["git", "init", str(guest)])
    run(["git", "remote", "add", "origin", GUEST_REPOSITORY], cwd=guest)
    run(["git", "fetch", "--depth", "1", "origin", GUEST_PORT_COMMIT], cwd=guest)
    run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=guest)
    got = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=guest, text=True
    ).strip().lower()
    if got != GUEST_PORT_COMMIT:
        raise RuntimeError(f"guest port-layer pin mismatch: {got}")
    return guest


def find_tool(root: Path, names: tuple[str, ...]) -> Path:
    bases = [
        root / "build" / "aros-local" / "toolchain",
        root / "build" / "aros-local" / "aros-build",
    ]
    for base in bases:
        if not base.exists():
            continue
        for name in names:
            found = next(
                (p for p in base.rglob(name) if p.is_file() and os.access(p, os.X_OK)),
                None,
            )
            if found:
                return found
    raise FileNotFoundError(
        f"AROS cross-tool not found: {', '.join(names)}. Run the AROS Kitchen Prepare first."
    )


def find_sysroot(root: Path) -> Path:
    aros_build = root / "build" / "aros-local" / "aros-build"
    candidates = (
        aros_build / "bin" / "amiga-m68k" / "AROS" / "Developer",
        aros_build / "bin" / "amiga-m68k" / "AROS" / "Development",
    )
    for candidate in candidates:
        if (candidate / "include").is_dir() and (candidate / "lib").is_dir():
            return candidate
    raise FileNotFoundError(
        "AROS m68k SDK sysroot not found. Run AROS Kitchen Prepare, then AGA Game Ports Prepare."
    )


def _replace_symlink(link: Path, target: Path) -> None:
    if link.is_symlink():
        if link.resolve() == target.resolve():
            return
        link.unlink()
    elif link.exists():
        marker = link / ".amigachrome-gameports-contrib"
        if not marker.is_file():
            raise RuntimeError(
                f"Refusing to replace unmanaged AROS contrib tree: {link}"
            )
        import shutil
        shutil.rmtree(link)
    link.parent.mkdir(parents=True, exist_ok=True)
    link.symlink_to(target.resolve(), target_is_directory=True)


def prepare_aros_game_sdk(root: Path) -> None:
    work = root / "build" / "game-ports"
    dep = work / "dependencies" / "arosContrib"
    if not dep.is_dir():
        raise RuntimeError("Pinned AROS contrib dependency is not prepared.")

    # Ensure the locked AROS source/toolchain exists first. Network is allowed
    # during Kitchen Prepare.
    run([str(root / "tools" / "aros-local" / "prepare.sh"), "--skip-boot"])

    aros_src = root / "build" / "aros-local" / "aros-src"
    aros_build = root / "build" / "aros-local" / "aros-build"
    if not aros_src.is_dir():
        raise RuntimeError("AROS source tree was not prepared.")

    _replace_symlink(aros_src / "contrib", dep)

    # contrib participates in the AROS make graph, so force one deterministic
    # reconfigure after staging it. The second prepare is offline.
    fingerprint = aros_build / ".amigachrome-config-fingerprint"
    fingerprint.unlink(missing_ok=True)
    run([str(root / "tools" / "aros-local" / "prepare.sh"), "--offline", "--skip-boot"])

    catalog = json.loads((root / "gameports" / "catalog.json").read_text(encoding="utf-8"))
    targets = catalog["buildDependencies"]["arosContrib"].get("prepareTargets") or []
    if not targets:
        raise RuntimeError("AROS contrib dependency has no prepareTargets.")
    run(["make", "-C", str(aros_build), "-j1", *[str(x) for x in targets]])

    sysroot = find_sysroot(root)
    required = (
        sysroot / "include" / "SDL2" / "SDL.h",
        sysroot / "include" / "png.h",
        sysroot / "include" / "zlib.h",
    )
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise RuntimeError("Prepared AROS SDL2 SDK is incomplete: " + ", ".join(missing))


def _write_cmake_toolchain(
    path: Path, cc: Path, sysroot: Path, cxx: Path | None = None
) -> None:
    cxx_line = f'set(CMAKE_CXX_COMPILER "{cxx}")\n' if cxx else ""
    cxx_flags = (
        f'set(CMAKE_CXX_FLAGS_INIT "--sysroot={sysroot} {CPU_FLAG} {NULL_FLAG}")\n'
        if cxx else ""
    )
    body = f"""set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "{cc}")
{cxx_line}set(CMAKE_C_FLAGS_INIT "--sysroot={sysroot} {CPU_FLAG} {NULL_FLAG}")
{cxx_flags}set(CMAKE_EXE_LINKER_FLAGS_INIT "--sysroot={sysroot} {CPU_FLAG}")
set(CMAKE_FIND_ROOT_PATH "{sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body, encoding="utf-8")


def _find_built_file(root: Path, name: str) -> Path:
    matches = [p for p in root.rglob(name) if p.is_file()]
    if not matches:
        raise FileNotFoundError(f"Built file not found: {name}")
    matches.sort(key=lambda p: (len(p.parts), str(p)))
    return matches[0]


def build_cdogs(root: Path, guest: Path, cc: Path, sysroot: Path, out: Path) -> dict:
    import shutil
    import zipfile

    prepared = json.loads(
        (root / "build" / "game-ports" / "PREPARED.json").read_text(encoding="utf-8")
    )
    source = Path(prepared["ports"]["cdogs-sdl"]["path"]).resolve()
    work = root / "build" / "game-ports" / "work" / "cdogs-sdl"
    build = root / "build" / "game-ports" / "build-cdogs-sdl"
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(build, ignore_errors=True)
    shutil.copytree(source, work, symlinks=True)

    apply_script = guest / "gameports" / "cdogs-sdl" / "apply_aros_bootstrap.py"
    run([sys.executable, str(apply_script), str(work)])

    toolchain = root / "build" / "game-ports" / "cmake" / "aros-m68k.cmake"
    _write_cmake_toolchain(toolchain, cc, sysroot)
    configure = [
        "cmake", "-S", str(work), "-B", str(build),
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        "-DAMIGA_AROS=ON",
        f"-DAROS_SDK={sysroot}",
        "-DBUILD_EDITOR=OFF",
        "-DBUILD_TESTING=OFF",
        "-DUSE_SHARED_ENET=OFF",
        "-DCDOGS_DATA_DIR=./",
    ]
    if shutil.which("ninja"):
        configure.extend(["-G", "Ninja"])
    run(configure)
    run(["cmake", "--build", str(build), "--target", "cdogs-sdl"])

    built = _find_built_file(build, "cdogs-sdl")
    binary = out / "cdogs-sdl-aros"
    shutil.copy2(built, binary)

    package = out / "cdogs-sdl-aga-bootstrap.zip"
    package_root = "C-Dogs-AGA-bootstrap"
    include_dirs = ("data", "missions", "dogfights", "graphics", "music", "sounds", "doc")
    include_files = ("README.md", "COPYING")
    with zipfile.ZipFile(package, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        zf.write(binary, f"{package_root}/cdogs-sdl")
        for name in include_files:
            path = work / name
            if path.is_file():
                zf.write(path, f"{package_root}/{name}")
        for dirname in include_dirs:
            base = work / dirname
            if not base.is_dir():
                continue
            for path in sorted(p for p in base.rglob("*") if p.is_file()):
                zf.write(path, f"{package_root}/{path.relative_to(work).as_posix()}")
        zf.writestr(
            f"{package_root}/AMIGACHROME-PORT.txt",
            "AROS/m68k bootstrap build. SDL2 renderer first; native ACGame AGA presentation follows.\n"
            f"Upstream pin: 8263a6f0200a71498f8c47bd0fce0f2d2d678d84\n",
        )

    return {
        "state": "done",
        "binary": str(binary),
        "package": str(package),
        "binarySha256": sha256(binary),
        "packageSha256": sha256(package),
    }


def build_hurrican(
    root: Path, guest: Path, cc: Path, cxx: Path, sysroot: Path, out: Path
) -> dict:
    import shutil

    prepared = json.loads(
        (root / "build" / "game-ports" / "PREPARED.json").read_text(encoding="utf-8")
    )
    source = Path(prepared["ports"]["hurrican"]["path"]).resolve()
    work = root / "build" / "game-ports" / "work" / "hurrican"
    build = root / "build" / "game-ports" / "build-hurrican"
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(build, ignore_errors=True)
    shutil.copytree(source, work, symlinks=True)

    apply_script = guest / "gameports" / "hurrican" / "apply_aros_gl_bootstrap.py"
    run([sys.executable, str(apply_script), str(work)])

    toolchain = root / "build" / "game-ports" / "cmake" / "aros-m68k-cxx.cmake"
    _write_cmake_toolchain(toolchain, cc, sysroot, cxx)
    configure = [
        "cmake", "-S", str(work / "Hurrican"), "-B", str(build),
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        "-DPLATFORM=AMIGA_AROS",
        "-DAMIGA_AROS=ON",
        f"-DAROS_SDK={sysroot}",
        "-DRENDERER=GL1",
        "-DFBO=OFF",
        "-DOPENMPT=OFF",
        "-DUSE_PRECOMPILED_HEADERS=OFF",
        "-DDISABLE_EXCEPTIONS=ON",
    ]
    if shutil.which("ninja"):
        configure.extend(["-G", "Ninja"])
    run(configure)
    run(["cmake", "--build", str(build), "--target", "hurrican"])

    built = _find_built_file(build, "hurrican")
    binary = out / "hurrican-aros"
    shutil.copy2(built, binary)

    run_note = out / "HURRICAN_RUN.txt"
    data_root = work / "Hurrican"
    run_note.write_text(
        "Hurrican AROS/m68k bootstrap (GL1)\n"
        "This build intentionally does not redistribute the Hurrican asset tree.\n"
        f"Run with the prepared source data at: {data_root}\n"
        f"Suggested data path: {data_root}\n"
        "Tracker music may be unavailable in the first m68k SDL2_mixer build.\n",
        encoding="utf-8",
    )
    return {
        "state": "done",
        "binary": str(binary),
        "runNote": str(run_note),
        "binarySha256": sha256(binary),
    }


def build_openomf(
    root: Path, guest: Path, cc: Path, sysroot: Path, out: Path
) -> dict:
    import shutil

    prepared = json.loads(
        (root / "build" / "game-ports" / "PREPARED.json").read_text(encoding="utf-8")
    )
    source = Path(prepared["ports"]["openomf"]["path"]).resolve()
    cdogs_source = Path(prepared["ports"]["cdogs-sdl"]["path"]).resolve()
    work = root / "build" / "game-ports" / "work" / "openomf"
    build = root / "build" / "game-ports" / "build-openomf"
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(build, ignore_errors=True)
    shutil.copytree(source, work, symlinks=True)

    enet_src = cdogs_source / "src" / "cdogs" / "enet"
    if not enet_src.is_dir():
        raise FileNotFoundError(f"Pinned C-Dogs ENet source missing: {enet_src}")
    vendored = work / "src" / "vendored"
    shutil.copytree(enet_src, vendored / "enet", dirs_exist_ok=True)

    acgame_src = guest / "gameports" / "acgame"
    shutil.copytree(acgame_src, vendored / "acgame", dirs_exist_ok=True)

    apply_script = guest / "gameports" / "openomf" / "apply_aga_profile.py"
    overlay = guest / "gameports" / "openomf" / "overlay"
    run([
        sys.executable, str(apply_script), str(work),
        "--overlay", str(overlay),
    ])

    toolchain = root / "build" / "game-ports" / "cmake" / "aros-m68k.cmake"
    _write_cmake_toolchain(toolchain, cc, sysroot)
    configure = [
        "cmake", "-S", str(work), "-B", str(build),
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        "-DAMIGAAGA=ON",
        f"-DAROS_SDK={sysroot}",
        "-DUSE_TESTS=OFF",
        "-DUSE_TOOLS=OFF",
        "-DBUILD_LANGUAGES=OFF",
        "-DUSE_LIBPNG=OFF",
        "-DUSE_OPUSFILE=OFF",
        "-DUSE_MINIUPNPC=OFF",
        "-DUSE_NATPMP=OFF",
        "-DUSE_EXTENDED_PALETTE=OFF",
    ]
    if shutil.which("ninja"):
        configure.extend(["-G", "Ninja"])
    run(configure)
    run(["cmake", "--build", str(build), "--target", "openomf"])

    built = _find_built_file(build, "openomf")
    binary = out / "openomf-aros-aga"
    shutil.copy2(built, binary)

    run_note = out / "OPENOMF_RUN.txt"
    engine_resources = work / "resources"
    run_note.write_text(
        "OpenOMF native AGA first-light build\n"
        "Renderer: ACGame indexed AGA (no OpenGL/Epoxy)\n"
        "Audio: NULL backend for first light\n"
        "Networking: pinned ENet retained\n"
        "Original One Must Fall 2097 game data is NOT bundled.\n"
        f"Engine resource source: {engine_resources}\n"
        "Place user-supplied OMF2097 data under PROGDIR:resources before launch.\n",
        encoding="utf-8",
    )
    return {
        "state": "done",
        "binary": str(binary),
        "runNote": str(run_note),
        "binarySha256": sha256(binary),
        "renderer": "ACGame AGA",
        "audio": "null-first-light",
    }


def _link_tool_wrapper(path: Path, target: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink() or path.exists():
        path.unlink()
    path.symlink_to(target.resolve())


def build_uqm(
    root: Path, guest: Path, cc: Path, cxx: Path, sysroot: Path, out: Path
) -> dict:
    import shutil

    prepared = json.loads(
        (root / "build" / "game-ports" / "PREPARED.json").read_text(encoding="utf-8")
    )
    source = Path(prepared["ports"]["uqm"]["path"]).resolve()
    work = root / "build" / "game-ports" / "work" / "uqm"
    build_work = root / "build" / "game-ports" / "build-uqm"
    wrappers = root / "build" / "game-ports" / "toolwrap-uqm"
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(build_work, ignore_errors=True)
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(source, work, symlinks=True)
    build_work.mkdir(parents=True, exist_ok=True)

    apply_script = guest / "gameports" / "uqm" / "apply_aros_bootstrap.py"
    config_state = build_work / "config.state"
    run([
        sys.executable, str(apply_script), str(work),
        "--write-config", str(config_state),
    ])

    if (work / "build.sh").is_file():
        sc2 = work
    elif (work / "sc2" / "build.sh").is_file():
        sc2 = work / "sc2"
    else:
        raise RuntimeError("UQM patched source root could not be located.")

    _link_tool_wrapper(wrappers / "gcc", cc)
    _link_tool_wrapper(wrappers / "cc", cc)
    _link_tool_wrapper(wrappers / "g++", cxx)
    _link_tool_wrapper(wrappers / "c++", cxx)
    try:
        _link_tool_wrapper(
            wrappers / "ar",
            find_tool(root, ("m68k-aros-ar", "m68k-unknown-aros-ar")),
        )
    except FileNotFoundError:
        pass
    try:
        _link_tool_wrapper(
            wrappers / "ranlib",
            find_tool(root, ("m68k-aros-ranlib", "m68k-unknown-aros-ranlib")),
        )
    except FileNotFoundError:
        pass

    env = os.environ.copy()
    env.update({
        "PATH": str(wrappers) + os.pathsep + env.get("PATH", ""),
        "BUILD_HOST": "AROS",
        "BUILD_HOST_ENDIAN": "big",
        "AROS_SDK": str(sysroot),
        "BUILD_WORK": str(build_work),
        "CFLAGS": f"--sysroot={sysroot} {CPU_FLAG} {NULL_FLAG}",
        "CXXFLAGS": f"--sysroot={sysroot} {CPU_FLAG} {NULL_FLAG}",
        "LDFLAGS": f"--sysroot={sysroot} {CPU_FLAG}",
    })
    run(["/bin/sh", "build.sh", "uqm"], cwd=sc2, env=env)

    built = build_work / "uqm"
    if not built.is_file():
        built = _find_built_file(build_work, "uqm")
    binary = out / "uqm-aros"
    shutil.copy2(built, binary)

    run_note = out / "UQM_RUN.txt"
    run_note.write_text(
        "The Ur-Quan Masters 0.8.0 AROS/m68k bootstrap\n"
        "Graphics: SDL2 first-light path\n"
        "Sound: internal MixSDL; Ogg codec disabled for first light\n"
        "Netplay: disabled\n"
        "Acceleration: portable C\n"
        "UQM content is NOT bundled.\n"
        "Launch with: uqm-aros -n <path-to-extracted-UQM-0.8-content>\n"
        "The official source archive remains the SHA-pinned build authority.\n",
        encoding="utf-8",
    )
    return {
        "state": "done",
        "binary": str(binary),
        "runNote": str(run_note),
        "binarySha256": sha256(binary),
        "graphics": "SDL2-bootstrap",
        "content": "external",
    }


def build_offline(root: Path) -> None:
    prepare = root / "scripts" / "game_port_prepare.py"
    run([
        sys.executable, str(prepare), "verify",
        "--root", str(root), "--game", "all",
    ])
    guest = root / "build" / "game-ports" / "port-layer"
    if not guest.is_dir():
        raise RuntimeError("Port layer was not prepared; run Prepare first.")
    got = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=guest, text=True
    ).strip().lower()
    if got != GUEST_PORT_COMMIT:
        raise RuntimeError("Port layer changed after Prepare.")

    cc = find_tool(root, ("m68k-aros-gcc", "m68k-unknown-aros-gcc"))
    cxx = find_tool(root, ("m68k-aros-g++", "m68k-unknown-aros-g++"))
    ar = find_tool(root, ("m68k-aros-ar", "m68k-unknown-aros-ar"))
    sysroot = find_sysroot(root)
    acgame = guest / "gameports" / "acgame"
    out = root / "build" / "game-ports" / OUTPUT_DIR_NAME
    out.mkdir(parents=True, exist_ok=True)
    c2p = out / f"c2p_ref-{GAMEPORT_CPU_TARGET}.o"
    ham = out / f"ham8-{GAMEPORT_CPU_TARGET}.o"
    platform = out / f"platform_aros_aga-{GAMEPORT_CPU_TARGET}.o"
    smoke_obj = out / f"aga_smoke-{GAMEPORT_CPU_TARGET}.o"
    lib = out / f"libacgame-{GAMEPORT_CPU_TARGET}.a"
    smoke = out / "acgame-aga-smoke"
    common = [
        str(cc), f"--sysroot={sysroot}", "-std=gnu11", "-O2", CPU_FLAG, NULL_FLAG,
        "-Wall", "-Wextra", "-Werror", "-Wno-volatile-register-var",
        "-I", str(acgame / "include"),
    ]
    run(common + ["-c", str(acgame / "src" / "c2p_ref.c"), "-o", str(c2p)])
    run(common + ["-c", str(acgame / "src" / "ham8.c"), "-o", str(ham)])
    run(common + ["-c", str(acgame / "src" / "platform_aros_aga.c"), "-o", str(platform)])
    run(common + ["-c", str(acgame / "examples" / "aga_smoke.c"), "-o", str(smoke_obj)])
    run([str(ar), "rcs", str(lib), str(c2p), str(ham), str(platform)])
    # AROS GCC's target specs supply the standard system libraries, including
    # intuition/graphics. Keeping the link line minimal also exercises the SDK
    # exactly as a normal external AROS program would.
    run([
        str(cc), f"--sysroot={sysroot}", CPU_FLAG,
        str(smoke_obj), str(lib), "-o", str(smoke),
    ])

    game_status = {
        "schema": 1,
        "target": GAME_TARGET,
        "canonicalTarget": f"m68k-aros/{CANONICAL_CPU_TARGET}",
        "compatibilityBuild": GAMEPORT_CPU_TARGET != CANONICAL_CPU_TARGET,
        "games": {},
    }
    try:
        game_status["games"]["cdogs-sdl"] = build_cdogs(
            root, guest, cc, sysroot, out
        )
    except Exception as exc:
        game_status["games"]["cdogs-sdl"] = {
            "state": "failed",
            "error": str(exc),
        }
    try:
        game_status["games"]["hurrican"] = build_hurrican(
            root, guest, cc, cxx, sysroot, out
        )
    except Exception as exc:
        game_status["games"]["hurrican"] = {
            "state": "failed",
            "error": str(exc),
        }
    try:
        game_status["games"]["openomf"] = build_openomf(
            root, guest, cc, sysroot, out
        )
    except Exception as exc:
        game_status["games"]["openomf"] = {
            "state": "failed",
            "error": str(exc),
        }
    try:
        game_status["games"]["uqm"] = build_uqm(
            root, guest, cc, cxx, sysroot, out
        )
    except Exception as exc:
        game_status["games"]["uqm"] = {
            "state": "failed",
            "error": str(exc),
        }
    game_status_path = out / "GAME_BUILD_STATUS.json"
    game_status_path.write_text(
        json.dumps(game_status, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    prepared = json.loads(
        (root / "build" / "game-ports" / "PREPARED.json").read_text(encoding="utf-8")
    )
    provenance = {
        "schema": 1,
        "target": GAME_TARGET,
        "canonicalTarget": f"m68k-aros/{CANONICAL_CPU_TARGET}",
        "compatibilityBuild": GAMEPORT_CPU_TARGET != CANONICAL_CPU_TARGET,
        "guestPortCommit": GUEST_PORT_COMMIT,
        "compiler": str(cc),
        "sysroot": str(sysroot),
        "artifacts": {p.name: sha256(p) for p in (c2p, ham, platform, smoke_obj, lib, smoke)},
        "preparedSources": prepared,
        "gameBuildStatus": game_status,
    }
    prov = out / "BUILD_PROVENANCE.json"
    prov.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    # Always leave a launch-oriented field-test tree, even when one or more
    # game lanes fail. Successful lanes stay testable instead of being hidden
    # behind the aggregate non-zero exit status.
    field_test_script = root / "scripts" / "stage_game_ports_for_test.py"
    run([sys.executable, str(field_test_script), "--root", str(root)])

    sums = out / "SHA256SUMS"
    with sums.open("w", encoding="utf-8") as fh:
        checksum_paths = [c2p, ham, platform, smoke_obj, lib, smoke, game_status_path, prov]
        for optional in (
            out / "cdogs-sdl-aros",
            out / "cdogs-sdl-aga-bootstrap.zip",
            out / "hurrican-aros",
            out / "HURRICAN_RUN.txt",
            out / "openomf-aros-aga",
            out / "OPENOMF_RUN.txt",
            out / "uqm-aros",
            out / "UQM_RUN.txt",
            out / "field-test" / "FIELD_TEST_MANIFEST.json",
            out / "field-test" / "FIELD_TEST_SHA256SUMS",
            out / "field-test" / "FIRST_LIGHT_TEST_PLAN.txt",
        ):
            if optional.is_file():
                checksum_paths.append(optional)
        for path in checksum_paths:
            fh.write(f"{sha256(path)}  {path.name}\n")

    failures = {
        name: item.get("error") or "unknown failure"
        for name, item in game_status["games"].items()
        if item.get("state") != "done"
    }
    if failures:
        summary = "; ".join(f"{name}: {error}" for name, error in failures.items())
        raise RuntimeError(
            "Game-port build completed with one or more failed lanes; "
            "successful artifacts were preserved. " + summary
        )


def run_action(root: Path, action: str) -> int:
    _, job_path, _ = paths(root)
    try:
        if action == "check":
            run([
                sys.executable, str(root / "scripts" / "game_port_prepare.py"),
                "check", "--root", str(root), "--game", "all",
            ])
        elif action == "prepare":
            run([
                sys.executable, str(root / "scripts" / "game_port_prepare.py"),
                "prepare", "--root", str(root), "--game", "all",
            ])
            ensure_port_layer(root)
            prepare_aros_game_sdk(root)
        else:
            build_offline(root)
    except Exception as exc:
        data = read_json(job_path, blank(root))
        data.update(
            state="failed",
            error=str(exc),
            message=str(exc),
            finishedAt=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        )
        write_json(job_path, data)
        print(str(exc), file=sys.stderr)
        return 1

    data = read_json(job_path, blank(root))
    data.update(
        state="done",
        error=None,
        message=ACTIONS[action] + " complete.",
        finishedAt=time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    )
    write_json(job_path, data)
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", type=Path, required=True)
    ap.add_argument("--run", choices=sorted(ACTIONS))
    args = ap.parse_args(argv)
    root = args.root.expanduser().resolve()
    return run_action(root, args.run) if args.run else 0


if __name__ == "__main__":
    raise SystemExit(main())

