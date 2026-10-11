#!/usr/bin/env python3
"""Instrument AssaultCube's first main-loop iteration on AmigaOS.

The client now reaches `init: mainloop` but does not present its first frame.
This patch disables SDL relative-mouse capture for the current Amiga first-light
build and records the exact first loop operation reached, using guest-owned
PROGDIR marker files added by patch_runtime_diagnostics.py.
"""

from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_mainloop_diagnostics.py ASSAULTCUBE_SOURCE_DIR")

source_dir = Path(sys.argv[1]).resolve()
source = source_dir / "src" / "main.cpp"
if not source.is_file():
    raise RuntimeError(f"AssaultCube main.cpp missing: {source}")

text = source.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    if new in text:
        return
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}")
    text = text.replace(old, new, 1)


replace_once(
    '''    inputgrab(grabinput = true);

    inmainloop = true;
''',
    '''    ACStage("PROGDIR:stage-ac-11-before-inputgrab");
#ifdef __amigaos__
    // OpenInput relative capture is not yet required for first light.  Keep
    // the normal windowed mouse path active and prove that the frame loop is
    // healthy before qualifying capture/release separately.
    userelativemouse = 0;
#endif
    inputgrab(grabinput = true);
    ACStage("PROGDIR:stage-ac-12-after-inputgrab");

    inmainloop = true;
    ACStage("PROGDIR:stage-ac-13-inmainloop-set");
''',
    "input grab boundary",
)

replace_once(
    '''    for(;;)
    {
        static int frames = 0;
        static float fps = 10.0f;
        int millis = SDL_GetTicks() - clockrealbase;
''',
    '''    for(;;)
    {
        static int frames = 0;
        static float fps = 10.0f;
        static bool acTraceFirstLoop = true;
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-20-loop-top");
        int millis = SDL_GetTicks() - clockrealbase;
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-21-ticks-read");
''',
    "loop top",
)

replace_once(
    '''        limitfps(millis, totalmillis);
        int elapsed = millis-totalmillis;
''',
    '''        limitfps(millis, totalmillis);
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-22-limitfps-returned");
        int elapsed = millis-totalmillis;
''',
    "frame limiter",
)

replace_once(
    '''        checkinput();

        if(lastmillis) updateworld(curtime, lastmillis);
''',
    '''        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-23-before-checkinput");
        checkinput();
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-24-after-checkinput");

        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-25-before-updateworld");
        if(lastmillis) updateworld(curtime, lastmillis);
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-26-after-updateworld");
''',
    "input and world",
)

replace_once(
    '''        serverslice(0);

        if(elapsed) fps = (1000.0f/elapsed+fps*10)/11; // avoid DIV-by-0
''',
    '''        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-27-before-serverslice");
        serverslice(0);
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-28-after-serverslice");

        if(elapsed) fps = (1000.0f/elapsed+fps*10)/11; // avoid DIV-by-0
''',
    "server slice",
)

replace_once(
    '''        audiomgr.updateaudio();

        computeraytable(camera1->o.x, camera1->o.y, dynfov());
''',
    '''        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-29-before-audio-update");
        audiomgr.updateaudio();
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-30-after-audio-update");

        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-31-before-raytable");
        computeraytable(camera1->o.x, camera1->o.y, dynfov());
        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-32-after-raytable");
''',
    "audio and ray table",
)

replace_once(
    '''        if(frames>3 && !minimized)
        {
            gl_drawframe(screenw, screenh, fps<lowfps ? fps/lowfps : (fps>highfps ? fps/highfps : 1.0f), fps, elapsed);
''',
    '''        if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-33-before-frame-branch");
        if(frames>3 && !minimized)
        {
            if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-34-before-drawframe");
            gl_drawframe(screenw, screenh, fps<lowfps ? fps/lowfps : (fps>highfps ? fps/highfps : 1.0f), fps, elapsed);
            if(acTraceFirstLoop) ACStage("PROGDIR:stage-ac-35-after-drawframe");
''',
    "draw frame boundary",
)

replace_once(
    '''        pollautodownloadresponse();
        if(onceauthed && sk)
''',
    '''        if(acTraceFirstLoop)
        {
            ACStage("PROGDIR:stage-ac-36-first-loop-complete");
            acTraceFirstLoop = false;
        }
        pollautodownloadresponse();
        if(onceauthed && sk)
''',
    "loop completion",
)

source.write_text(text, encoding="utf-8")
print(f"patched AssaultCube first-loop diagnostics: {source}")
