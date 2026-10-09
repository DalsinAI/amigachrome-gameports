#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import socket
import struct
import subprocess
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACROOT = Path.home() / "AmigaChrome"
INSTANCES = Path.home() / "AmigaChromeInstances"
LAB_TOOL = ACROOT / "scripts" / "lab_instance.py"
RUN_ROOT = Path.home() / "AmigaChrome-dev" / ("gameports-runtime-" + os.environ.get("GITHUB_RUN_ID", str(int(time.time()))))
RESULT_ROOT = ROOT / "build" / "runtime-smoke"
PAYLOAD_ROOT = ROOT / "build" / "runtime-payload"

ESC = 0x45
INPUT_SCRIPT = ""

def run(cmd, *, cwd=None, check=True, env=None, capture=False):
    print("+", " ".join(str(x) for x in cmd), flush=True)
    return subprocess.run(cmd, cwd=cwd, check=check, env=env, text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.STDOUT if capture else None)

def ci_child(base: Path, name: str) -> Path:
    if not base.is_dir():
        return base / name
    low = name.lower()
    for p in base.iterdir():
        if p.name.lower() == low:
            return p
    return base / name

def dh0_of(inst: Path) -> Path:
    return ci_child(ci_child(ci_child(inst, "devices"), "harddisks"), "DH0")

def user_startup(inst: Path) -> Path:
    s = ci_child(dh0_of(inst), "S")
    s.mkdir(parents=True, exist_ok=True)
    return ci_child(s, "User-Startup")

def port_open(port: int) -> bool:
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=0.15):
            return True
    except OSError:
        return False

def instance_score(inst: Path) -> tuple[int, str]:
    try:
        cfg = json.loads((inst / "config" / "local.json").read_text())
    except Exception:
        return (-9999, inst.name)
    dh0 = dh0_of(inst)
    if not (dh0 / "S").is_dir() and not ci_child(dh0, "S").is_dir():
        return (-9999, inst.name)
    score = 0
    port = int(cfg.get("runtimePort", 0) or 0)
    if port and not port_open(port):
        score += 20
    libs = ci_child(dh0, "Libs")
    names = {p.name.lower() for p in libs.iterdir()} if libs.is_dir() else set()
    for wanted in ("opengpu.library", "openinput.library", "opentls.library"):
        if wanted in names:
            score += 5
    if inst.name in ("Instance-22", "Instance-24", "Instance-11"):
        score += {"Instance-22": 4, "Instance-24": 3, "Instance-11": 2}[inst.name]
    return (score, inst.name)

def choose_source() -> Path:
    requested = os.environ.get("GAMEPORTS_SOURCE_INSTANCE")
    if requested:
        p = Path(requested).expanduser()
        if not p.is_absolute():
            p = INSTANCES / requested
        if p.is_dir():
            return p.resolve()
    candidates = [p for p in INSTANCES.glob("Instance-*") if (p / "config" / "local.json").is_file()]
    ranked = sorted(((instance_score(p), p) for p in candidates), reverse=True)
    if not ranked or ranked[0][0][0] < 0:
        raise RuntimeError("no usable AmigaChrome instance found for lab source")
    print("source candidates:", [(s, p.name) for s, p in ranked[:8]])
    return ranked[0][1].resolve()

def download(url: str, target: Path) -> bool:
    target.parent.mkdir(parents=True, exist_ok=True)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "AmigaChrome-gameports-runtime-smoke/1"})
        with urllib.request.urlopen(req, timeout=60) as r, target.open("wb") as f:
            shutil.copyfileobj(r, f)
        return target.stat().st_size > 0
    except Exception as e:
        print("download failed:", url, e)
        return False

def package_payloads():
    shutil.rmtree(PAYLOAD_ROOT, ignore_errors=True)
    PAYLOAD_ROOT.mkdir(parents=True)

    out = {}

    # Neverball is already a complete release-style package.
    nb = ROOT / "build/os3/neverball-release/package/Neverball"
    if nb.is_dir():
        shutil.copytree(nb, PAYLOAD_ROOT / "Neverball")
        out["neverball"] = {"dir": "Neverball", "command": "Neverball", "graphics": True, "data": "official"}

    # C-Dogs: build the engineering-only self-contained package from the pinned tree.
    cd_bin = ROOT / "build/os3/cdogs/src/cdogs-sdl"
    cd_src = ROOT / "build/game-ports/sources/cdogs-sdl"
    if cd_bin.is_file() and cd_src.is_dir():
        pkg = ROOT / "build/runtime-package-cdogs"
        run(["sh", str(ROOT / "ports/cdogs-sdl/package-engineering.sh"), str(cd_src), str(cd_bin), str(pkg)])
        src = pkg / "C-Dogs"
        if src.is_dir():
            shutil.copytree(src, PAYLOAD_ROOT / "CDogs")
            out["cdogs"] = {"dir": "CDogs", "command": "CDogs", "graphics": True, "data": "upstream engineering data"}

    # Chocolate Doom + redistributable Freedoom qualification data.
    doom_bin = ROOT / "build/os3/chocolate-doom/src/chocolate-doom"
    if doom_bin.is_file():
        d = PAYLOAD_ROOT / "ChocolateDoom"; d.mkdir()
        shutil.copy2(doom_bin, d / "chocolate-doom")
        z = ROOT / "build/test-data/freedoom-0.13.0.zip"
        ok = download("https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip", z)
        if ok and zipfile.is_zipfile(z):
            with zipfile.ZipFile(z) as zf:
                with zf.open("freedoom-0.13.0/freedoom1.wad") as src, (d / "freedoom1.wad").open("wb") as dst:
                    shutil.copyfileobj(src, dst)
            out["chocolate-doom"] = {"dir": "ChocolateDoom", "command": "chocolate-doom -iwad freedoom1.wad", "graphics": True, "data": "Freedoom 0.13.0"}
        else:
            out["chocolate-doom"] = {"blocked": "Freedoom test data unavailable"}

    # SDLPoP carries its normal runtime data in the pinned source tree.
    pop_bin = ROOT / "build/game-ports/sources/sdlpop/prince"
    pop_src = ROOT / "build/game-ports/sources/sdlpop"
    if pop_bin.is_file() and (pop_src / "data").is_dir():
        d = PAYLOAD_ROOT / "SDLPoP"; d.mkdir()
        shutil.copy2(pop_bin, d / "prince")
        shutil.copytree(pop_src / "data", d / "data")
        if (pop_src / "SDLPoP.ini").is_file(): shutil.copy2(pop_src / "SDLPoP.ini", d / "SDLPoP.ini")
        out["sdlpop"] = {"dir": "SDLPoP", "command": "prince", "graphics": True, "data": "project runtime data"}

    # UQM official base content.
    uqm_bin_candidates = list((ROOT / "build/os3/uqm").rglob("uqm*")) if (ROOT / "build/os3/uqm").is_dir() else []
    uqm_bin = next((p for p in uqm_bin_candidates if p.is_file() and os.access(p, os.X_OK)), None)
    if uqm_bin:
        d = PAYLOAD_ROOT / "UQM"; (d / "content/packages").mkdir(parents=True)
        shutil.copy2(uqm_bin, d / "uqm")
        pkg = ROOT / "build/test-data/uqm-0.8.0-content.uqm"
        if download("https://downloads.sourceforge.net/project/sc2/UQM/0.8/uqm-0.8.0-content.uqm", pkg):
            shutil.copy2(pkg, d / "content/packages/uqm-0.8.0-content.uqm")
            out["uqm"] = {"dir": "UQM", "command": "uqm -n content", "graphics": True, "data": "official UQM 0.8 base content"}
        else:
            out["uqm"] = {"blocked": "official UQM content unavailable"}

    # OpenJazz: use the documented shareware test source when it resolves to a zip.
    jazz_bin = ROOT / "build/os3/openjazz/OpenJazz"
    if jazz_bin.is_file():
        d = PAYLOAD_ROOT / "OpenJazz"; d.mkdir()
        shutil.copy2(jazz_bin, d / "OpenJazz")
        z = ROOT / "build/test-data/jazz-shareware-1.1.zip"
        if download("https://www.dosgamesarchive.com/file.php?id=2031", z) and zipfile.is_zipfile(z):
            with zipfile.ZipFile(z) as zf: zf.extractall(d)
            out["openjazz"] = {"dir": "OpenJazz", "command": "OpenJazz", "graphics": True, "data": "Jazz Jackrabbit 1.1 shareware"}
        else:
            out["openjazz"] = {"blocked": "shareware test data unavailable"}

    # AssaultCube server is useful without rendering. Client gets whatever data is in the pinned source tree.
    acroot = ROOT / "build/game-ports/sources/assaultcube"
    server = acroot / "source/src/ac_server"
    client = acroot / "source/src/ac_client"
    if server.is_file():
        d = PAYLOAD_ROOT / "AssaultCubeServer"; d.mkdir()
        shutil.copy2(server, d / "ac_server")
        out["assaultcube-server"] = {"dir": "AssaultCubeServer", "command": "ac_server", "graphics": False, "data": "none"}
    if client.is_file():
        d = PAYLOAD_ROOT / "AssaultCube"; d.mkdir()
        shutil.copy2(client, d / "ac_client")
        copied = []
        for name in ("packages", "config", "demos", "mods"):
            src = acroot / name
            if src.is_dir():
                shutil.copytree(src, d / name)
                copied.append(name)
        if "packages" in copied:
            out["assaultcube-client"] = {"dir": "AssaultCube", "command": "ac_client", "graphics": True, "data": ",".join(copied)}
        else:
            out["assaultcube-client"] = {"blocked": "runtime packages absent from pinned source checkout"}

    # Explicit status for known campaign gaps.
    openomf_bin = ROOT / "build/os3/openomf/openomf"
    if openomf_bin.is_file():
        out["openomf"] = {"blocked": "build green; original OMF2097 runtime data is still required for qualification"}
    else:
        out["openomf"] = {"blocked": "build did not produce OpenOMF binary"}
    out["nxengine-evo"] = {"blocked": "not part of current fixed-GCC campaign"}

    return out

def amend_startup(lab: Path, rel_dir: str, command: str, slug: str):
    test_root = dh0_of(lab) / "GamePortsTest"
    test_root.mkdir(parents=True, exist_ok=True)
    target = test_root / rel_dir
    shutil.copytree(PAYLOAD_ROOT / rel_dir, target)
    status = test_root / (slug + ".status")
    us = user_startup(lab)
    old = us.read_text(encoding="latin-1", errors="replace") if us.is_file() else ""
    block = (
        "\n;BEGIN GamePorts Runtime Smoke\n"
        "FailAt 21\n"
        f"Echo BASELINE >DH0:GamePortsTest/{slug}.status\n"
        "Wait 60\n"
        f"Echo STARTED >>DH0:GamePortsTest/{slug}.status\n"
        f"CD DH0:GamePortsTest/{rel_dir}\n"
        f"{command} >DH0:GamePortsTest/{slug}.out\n"
        f"Echo RETURNED >>DH0:GamePortsTest/{slug}.status\n"
        ";END GamePorts Runtime Smoke\n"
    )
    us.write_text(old.rstrip() + block, encoding="latin-1")
    return status

def http_post(base: str, data: bytes):
    req = urllib.request.Request(base + "native/input", data=data, method="POST",
                                 headers={"Content-Type": "application/octet-stream"})
    with urllib.request.urlopen(req, timeout=2) as r:
        r.read()

def http_get(base: str, path: str) -> bytes:
    with urllib.request.urlopen(base + path, timeout=3) as r:
        return r.read()

def viewer_record():
    return struct.pack("<BBhhH", 6, 0, 0, 0, 0)

def debug_watch_record():
    return struct.pack("<BBhhH", 13, 0, 1, 0, 0)

def frame_info(data: bytes):
    if len(data) < 64 or data[:4] != b"ACF4":
        return None
    seq,w,h = struct.unpack_from("<3I", data, 4)
    x,y,vw,vh = struct.unpack_from("<4I", data, 16)
    tick,flags = struct.unpack_from("<2I", data, 32)
    need = 64 + vw * vh * 4
    full = len(data) >= need and vw > 0 and vh > 0
    info = {"seq":seq,"w":w,"h":h,"view":[x,y,vw,vh],"tick":tick,"flags":flags,"bytes":len(data),"full":full}
    if full:
        rows = data[64:need]
        info["pixelSha256"] = hashlib.sha256(rows).hexdigest()
        sample = [struct.unpack_from("<I", rows, i)[0] for i in range(0, len(rows)-3, max(4, (len(rows)//4096)//4*4 or 4))]
        info["sampleUnique"] = len(set(sample))
        info["sampleNonzero"] = sum(1 for p in sample if p & 0x00ffffff)
    return info

def frame_to_ppm(data: bytes, out: Path):
    info = frame_info(data)
    if not info or not info["full"]:
        return False
    vw,vh = info["view"][2], info["view"][3]
    rows = data[64:64+vw*vh*4]
    rgb = bytearray()
    for i in range(0, len(rows), 4):
        p = struct.unpack_from("<I", rows, i)[0]
        rgb += bytes(((p >> 16) & 255, (p >> 8) & 255, p & 255))
    out.write_bytes(f"P6\n{vw} {vh}\n255\n".encode() + rgb)
    return True

def fetch_frame(base: str, since: int):
    url = base + f"native/frame?since={since}&next=1&view=1"
    try:
        with urllib.request.urlopen(url, timeout=4) as r:
            if r.status == 204:
                return since, None
            data = r.read()
    except urllib.error.HTTPError as e:
        if e.code == 204:
            return since, None
        raise
    if len(data) < 32:
        return since, None

    magic = struct.unpack_from("<I", data, 0)[0]
    v4 = magic == 0x34464341
    v3 = v4 or magic == 0x33464341
    v2 = magic == 0x32464341
    off = 64 if v4 else 48 if v3 else 32 if v2 else 16
    hdr_count = 16 if v4 else 12 if v3 else 8
    if len(data) < min(off, hdr_count * 4):
        return since, None
    hdr = struct.unpack_from("<" + "I" * hdr_count, data, 0)
    seq = int(hdr[1])
    w, h = int(hdr[2]), int(hdr[3])
    vx = int(hdr[4]) if (v2 or v3) else 0
    vy = int(hdr[5]) if (v2 or v3) else 0
    vw = int(hdr[6]) if (v2 or v3) else w
    vh = int(hdr[7]) if (v2 or v3) else h
    flags = int(hdr[9]) if v3 and len(hdr) > 9 else 0
    tick = int(hdr[8]) if v3 and len(hdr) > 8 else 0
    if flags & 1:
        return seq, {"same": True, "seq": seq, "tick": tick}
    if w <= 0 or h <= 0 or vw <= 0 or vh <= 0:
        return seq, None
    need = off + w * h * 4
    if len(data) < need:
        return seq, {"bad": True, "seq": seq, "tick": tick,
                     "need": need, "bytes": len(data), "w": w, "h": h,
                     "view": [vx, vy, vw, vh], "flags": flags}

    cut = bool(flags & 2)
    sx, sy = (0, 0) if cut else (vx, vy)
    if sx + vw > w or sy + vh > h:
        return seq, {"bad": True, "seq": seq, "tick": tick,
                     "reason": "view-outside-frame", "w": w, "h": h,
                     "view": [vx, vy, vw, vh], "flags": flags}
    pixels = memoryview(data)[off:need]

    def pixel(x, y):
        at = ((sy + y) * w + (sx + x)) * 4
        return struct.unpack_from("<I", pixels, at)[0] & 0x00ffffff

    samples = []
    gx_n, gy_n = 80, 32
    for gy in range(gy_n):
        y = min(vh - 1, (gy * vh) // gy_n)
        for gx in range(gx_n):
            x = min(vw - 1, (gx * vw) // gx_n)
            samples.append(pixel(x, y))
    sample_bytes = b"".join(struct.pack("<I", p) for p in samples)
    return seq, {
        "same": False, "seq": seq, "tick": tick, "flags": flags,
        "w": w, "h": h, "view": [vx, vy, vw, vh],
        "samples": samples,
        "sampleUnique": len(set(samples)),
        "sampleSha256": hashlib.sha256(sample_bytes).hexdigest(),
        "_pixels": pixels.tobytes(), "_off": off, "_sx": sx, "_sy": sy,
    }


def frame_change(a, b):
    if not a or not b:
        return 0.0
    aa, bb = a.get("samples", []), b.get("samples", [])
    n = min(len(aa), len(bb))
    if not n:
        return 0.0
    return sum(1 for i in range(n) if aa[i] != bb[i]) / n


def public_frame(f):
    if not f:
        return None
    return {k: v for k, v in f.items() if not k.startswith("_") and k != "samples"}


def write_frame_ppm(f, out: Path):
    if not f or f.get("same") or f.get("bad") or "_pixels" not in f:
        return False
    w, h = int(f["w"]), int(f["h"])
    vx, vy, vw, vh = map(int, f["view"])
    sx, sy = int(f["_sx"]), int(f["_sy"])
    raw = f["_pixels"]
    rgb = bytearray(vw * vh * 3)
    o = 0
    for y in range(vh):
        for x in range(vw):
            at = ((sy + y) * w + (sx + x)) * 4
            p = struct.unpack_from("<I", raw, at)[0]
            rgb[o] = (p >> 16) & 255
            rgb[o + 1] = (p >> 8) & 255
            rgb[o + 2] = p & 255
            o += 3
    out.write_bytes(f"P6\n{vw} {vh}\n255\n".encode("ascii") + rgb)
    return True


def send_raw_key(base: str, raw: int):
    rec = struct.pack("<BBhhH", 1, 0, raw & 0xff, 0, 0)
    http_post(base, rec)


def runtime_smoke(source: Path, slug: str, spec: dict):
    result_dir = RESULT_ROOT / slug
    result_dir.mkdir(parents=True, exist_ok=True)
    lab = RUN_ROOT / slug
    if lab.exists():
        run(["python3", str(LAB_TOOL), "stop", str(lab)], check=False)
        shutil.rmtree(lab, ignore_errors=True)
    run(["python3", str(LAB_TOOL), "copy", str(source), str(lab), "--who", "Thufir"])
    marker = amend_startup(lab, spec["dir"], spec["command"], slug)
    output_file = dh0_of(lab) / "GamePortsTest" / (slug + ".out")
    envs = ["AC_PACE_HZ=0", "AC090_COUNTED=1", "AC_RTC_AT=1790960000",
            "AC_WATCHDOG=1", "JIT_HIST=1"]
    cmd = ["python3", str(LAB_TOOL), "start", str(lab), "--who", "Thufir",
           "--purpose", "game port runtime smoke " + slug]
    for e in envs:
        cmd += ["--env", e]
    started = run(cmd, capture=True)
    print(started.stdout or "")
    rec = json.loads((lab / "LAB.json").read_text())
    port = int(rec["run"]["bridgePort"])
    base = f"http://127.0.0.1:{port}/"

    observations = []
    seq = -1
    baseline = None
    last_full = None

    # Wait for a real Amiga picture while User-Startup is still in its
    # deliberate 60-second pre-launch hold.
    baseline_deadline = time.time() + 25
    while time.time() < baseline_deadline:
        try:
            seq, f = fetch_frame(base, seq)
            if f and not f.get("same") and not f.get("bad"):
                last_full = f
                if f.get("sampleUnique", 0) > 2:
                    baseline = f
                    break
        except Exception as e:
            observations.append("baseline-frame:" + repr(e))
        time.sleep(0.25)

    if baseline:
        write_frame_ppm(baseline, result_dir / "baseline.ppm")

    # Wait until the Amiga itself says it launched the program.
    marker_deadline = time.time() + 65
    while time.time() < marker_deadline:
        if marker.is_file():
            status_now = marker.read_text(encoding="latin-1", errors="replace")
            if "STARTED" in status_now:
                break
        time.sleep(0.25)
    started_marker = marker.is_file() and "STARTED" in marker.read_text(
        encoding="latin-1", errors="replace"
    )

    captures = []
    max_change = 0.0
    best_frame = None
    if started_marker:
        game_deadline = time.time() + 14
        next_sample = 0.0
        while time.time() < game_deadline:
            try:
                seq, f = fetch_frame(base, seq)
                if f and not f.get("same") and not f.get("bad"):
                    last_full = f
                    if time.time() >= next_sample:
                        d = frame_change(baseline, f)
                        captures.append({"changedFraction": round(d, 4),
                                         "frame": public_frame(f)})
                        if d > max_change:
                            max_change = d
                            best_frame = f
                        next_sample = time.time() + 0.7
            except Exception as e:
                observations.append("game-frame:" + repr(e))
                time.sleep(0.2)

        if best_frame:
            write_frame_ppm(best_frame, result_dir / "game-best.ppm")

        # Exercise the real native raw-key path: Escape down/up.
        before_key = last_full
        try:
            send_raw_key(base, ESC)
            time.sleep(0.08)
            send_raw_key(base, ESC | 0x80)
        except Exception as e:
            observations.append("escape-send:" + repr(e))
        input_change = 0.0
        key_deadline = time.time() + 2.5
        while time.time() < key_deadline:
            try:
                seq, f = fetch_frame(base, seq)
                if f and not f.get("same") and not f.get("bad"):
                    last_full = f
                    input_change = max(input_change, frame_change(before_key, f))
            except Exception as e:
                observations.append("post-key-frame:" + repr(e))
            time.sleep(0.1)
        if last_full:
            write_frame_ppm(last_full, result_dir / "after-escape.ppm")
    else:
        input_change = 0.0

    debug_text = ""
    try:
        http_post(base, debug_watch_record())
        debug_text = http_get(base, "native/debug").decode("utf-8", "replace")
        (result_dir / "native-debug.txt").write_text(debug_text, encoding="utf-8")
    except Exception as e:
        observations.append("debug:" + repr(e))

    status_text = marker.read_text(encoding="latin-1", errors="replace") if marker.is_file() else ""
    guest_output = output_file.read_text(encoding="latin-1", errors="replace") if output_file.is_file() else ""
    (result_dir / "guest-output.txt").write_text(guest_output, encoding="utf-8")
    native_log = lab / "logs/native-runtime.log"
    log_text = native_log.read_text(encoding="utf-8", errors="replace") if native_log.is_file() else ""
    (result_dir / "native-runtime.log").write_text(log_text, encoding="utf-8")
    for name in ("bridge-lab.log", "host-lab.log"):
        p = lab / "logs" / name
        if p.is_file():
            shutil.copy2(p, result_dir / name)

    crash_terms = ["Software Failure", "80000005", "TRAP #7", "divide by zero",
                   "Address Error", "Bus Error", "Illegal Instruction"]
    all_text = "\n".join((log_text, debug_text, guest_output))
    crashes = [term for term in crash_terms if term.lower() in all_text.lower()]
    returned = "RETURNED" in status_text
    graphics_alive = bool(spec.get("graphics") and started_marker and
                          baseline and max_change >= 0.08)
    if crashes:
        verdict = "FAIL_CRASH"
    elif not started_marker:
        verdict = "NO_START"
    elif not spec.get("graphics"):
        verdict = "FIRST_LIGHT"
    elif graphics_alive:
        verdict = "FIRST_LIGHT"
    else:
        verdict = "NO_VISIBLE_GAME_FRAME"

    result = {
        "slug": slug, "sourceInstance": str(source), "lab": str(lab), "bridgePort": port,
        "command": spec["command"], "data": spec.get("data"),
        "startedMarker": started_marker, "returned": returned,
        "graphicsExpected": bool(spec.get("graphics")),
        "graphicsObserved": graphics_alive,
        "baseline": public_frame(baseline),
        "maxFrameChangedFraction": round(max_change, 4),
        "escapeChangedFraction": round(input_change, 4),
        "crashTerms": crashes, "guestOutputTail": guest_output[-4000:],
        "observations": observations[-30:], "verdict": verdict,
    }
    (result_dir / "frame-samples.json").write_text(
        json.dumps(captures, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (result_dir / "result.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    run(["python3", str(LAB_TOOL), "stop", str(lab)], check=False)
    return result

def main():
    from concurrent.futures import ThreadPoolExecutor, as_completed

    ap = argparse.ArgumentParser()
    ap.add_argument("--source")
    args = ap.parse_args()
    shutil.rmtree(RESULT_ROOT, ignore_errors=True)
    RESULT_ROOT.mkdir(parents=True, exist_ok=True)
    source = Path(args.source).expanduser().resolve() if args.source else choose_source()
    payloads = package_payloads()
    results = {}
    runnable = {}

    for slug, spec in payloads.items():
        if "blocked" in spec:
            results[slug] = {"slug": slug, "verdict": "BLOCKED", "reason": spec["blocked"]}
        else:
            runnable[slug] = spec

    # Labs have separate disks and the lab tool allocates separate port blocks.
    # Three at once keeps disk-copy pressure reasonable while using the machine
    # we actually designed for parallel work.
    with ThreadPoolExecutor(max_workers=min(3, max(1, len(runnable)))) as pool:
        future_slug = {pool.submit(runtime_smoke, source, slug, spec): slug
                       for slug, spec in runnable.items()}
        for future in as_completed(future_slug):
            slug = future_slug[future]
            try:
                results[slug] = future.result()
            except Exception as e:
                results[slug] = {"slug": slug, "verdict": "HARNESS_ERROR", "error": repr(e)}
                print(slug, "HARNESS ERROR", repr(e), flush=True)

    summary = {
        "schema": 1,
        "at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "sourceInstance": str(source),
        "results": dict(sorted(results.items())),
    }
    (RESULT_ROOT / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print(json.dumps(summary, indent=2, sort_keys=True))
    # Runtime failures are evidence, not a workflow infrastructure failure.
    return 0

if __name__=="__main__":
    raise SystemExit(main())
