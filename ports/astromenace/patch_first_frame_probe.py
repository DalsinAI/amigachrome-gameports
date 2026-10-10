#!/usr/bin/env python3
"""Add one-shot first-frame milestones to AstroMenace on AmigaOS.

Console output redirected by AmigaDOS can remain buffered while the game is
alive.  Small guest-owned marker files give the qualification runner an
unambiguous boundary without printing from every frame.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
loop_cpp = source / "src" / "loop_proc.cpp"
text = loop_cpp.read_text(encoding="utf-8")

if "stage-first-frame-presented" in text:
    print("AstroMenace first-frame probe already present")
    raise SystemExit(0)

include_anchor = '#include "SDL2/SDL.h"\n'
include_insert = '''#include "SDL2/SDL.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
if include_anchor not in text:
    raise RuntimeError("AstroMenace Loop_Proc SDL include marker missing")
text = text.replace(include_anchor, include_insert, 1)

namespace_anchor = '''namespace viewizard {
namespace astromenace {

'''
namespace_insert = '''namespace viewizard {
namespace astromenace {

#ifdef AMIGACHROME
static void AMFirstFrameStage(const char *Name)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}
#endif

'''
if namespace_anchor not in text:
    raise RuntimeError("AstroMenace Loop_Proc namespace marker missing")
text = text.replace(namespace_anchor, namespace_insert, 1)

entry_anchor = '''void Loop_Proc()
{
    CursorUpdate();
'''
entry_insert = '''void Loop_Proc()
{
#ifdef AMIGACHROME
    static bool FirstFrameProbe = true;
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-loop-enter");
#endif
    CursorUpdate();
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-cursor-updated");
#endif
'''
if entry_anchor not in text:
    raise RuntimeError("AstroMenace Loop_Proc entry marker missing")
text = text.replace(entry_anchor, entry_insert, 1)

begin_anchor = '''    vw_BeginRendering(RI_COLOR_BUFFER | RI_DEPTH_BUFFER);

    switch (MenuStatus) {
'''
begin_insert = '''    vw_BeginRendering(RI_COLOR_BUFFER | RI_DEPTH_BUFFER);
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-render-begun");
#endif

    switch (MenuStatus) {
'''
if begin_anchor not in text:
    raise RuntimeError("AstroMenace begin-render marker missing")
text = text.replace(begin_anchor, begin_insert, 1)

switch_anchor = '''    }

    vw_Start2DMode(-1,1);
'''
switch_insert = '''    }
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-menu-drawn");
#endif

    vw_Start2DMode(-1,1);
'''
if switch_anchor not in text:
    raise RuntimeError("AstroMenace post-menu marker missing")
text = text.replace(switch_anchor, switch_insert, 1)

overlay_anchor = '''    cFPS::GetInstance().Draw();

    vw_End2DMode();
    vw_EndRendering();
'''
overlay_insert = '''    cFPS::GetInstance().Draw();
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-overlays-drawn");
#endif

    vw_End2DMode();
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-2d-ended");
#endif
    vw_EndRendering();
#ifdef AMIGACHROME
    if (FirstFrameProbe) {
        AMFirstFrameStage("PROGDIR:stage-first-frame-presented");
        FirstFrameProbe = false;
    }
#endif
'''
if overlay_anchor not in text:
    raise RuntimeError("AstroMenace end-render marker missing")
text = text.replace(overlay_anchor, overlay_insert, 1)

loop_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace first-frame render milestones")
