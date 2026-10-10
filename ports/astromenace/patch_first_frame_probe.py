#!/usr/bin/env python3
"""Add one-shot frame milestones and capture the composed default framebuffer.

AstroMenace normally renders into an FBO and resolves it to the window inside
vw_EndRendering(). Reading pixels earlier captures the stale/default surface,
not the frame that is about to be presented. Arm a diagnostic capture from the
main loop, then perform it after the FBO resolve and before SDL_GL_SwapWindow.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
loop_cpp = source / "src" / "loop_proc.cpp"
graphics_h = source / "src" / "core" / "graphics" / "graphics.h"
gl_main_cpp = source / "src" / "core" / "graphics" / "gl_main.cpp"

loop_text = loop_cpp.read_text(encoding="utf-8")
if "stage-first-frame-captured" not in loop_text:
    include_anchor = '#include "SDL2/SDL.h"\n'
    include_insert = '''#include "SDL2/SDL.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
    if include_anchor not in loop_text:
        raise RuntimeError("AstroMenace Loop_Proc SDL include marker missing")
    loop_text = loop_text.replace(include_anchor, include_insert, 1)

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
    if namespace_anchor not in loop_text:
        raise RuntimeError("AstroMenace Loop_Proc namespace marker missing")
    loop_text = loop_text.replace(namespace_anchor, namespace_insert, 1)

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
    if entry_anchor not in loop_text:
        raise RuntimeError("AstroMenace Loop_Proc entry marker missing")
    loop_text = loop_text.replace(entry_anchor, entry_insert, 1)

    begin_anchor = '''    vw_BeginRendering(RI_COLOR_BUFFER | RI_DEPTH_BUFFER);

    switch (MenuStatus) {
'''
    begin_insert = '''    vw_BeginRendering(RI_COLOR_BUFFER | RI_DEPTH_BUFFER);
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-render-begun");
#endif

    switch (MenuStatus) {
'''
    if begin_anchor not in loop_text:
        raise RuntimeError("AstroMenace begin-render marker missing")
    loop_text = loop_text.replace(begin_anchor, begin_insert, 1)

    switch_anchor = '''    }

    vw_Start2DMode(-1,1);
'''
    switch_insert = '''    }
#ifdef AMIGACHROME
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-menu-drawn");
#endif

    vw_Start2DMode(-1,1);
'''
    if switch_anchor not in loop_text:
        raise RuntimeError("AstroMenace post-menu marker missing")
    loop_text = loop_text.replace(switch_anchor, switch_insert, 1)

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
    if (FirstFrameProbe) {
        AMFirstFrameStage("PROGDIR:stage-2d-ended");
        AMFirstFrameStage("PROGDIR:stage-frame-ready-for-readback");
        vw_ArmDiagnosticFrameCapture(
            "PROGDIR:AstroMenace-first-frame.bmp",
            "PROGDIR:stage-first-frame-captured",
            "PROGDIR:stage-first-frame-capture-failed");
    }
#endif
    vw_EndRendering();
#ifdef AMIGACHROME
    if (FirstFrameProbe) {
        AMFirstFrameStage("PROGDIR:stage-first-frame-presented");
        FirstFrameProbe = false;
    }
#endif
'''
    if overlay_anchor not in loop_text:
        raise RuntimeError("AstroMenace end-render marker missing")
    loop_text = loop_text.replace(overlay_anchor, overlay_insert, 1)
    loop_cpp.write_text(loop_text, encoding="utf-8")

graphics_text = graphics_h.read_text(encoding="utf-8")
if "vw_ArmDiagnosticFrameCapture" not in graphics_text:
    graphics_anchor = '''// Create screenshot from current OpenGL surface.
int vw_Screenshot(int Width, int Height, const std::string &FileName);
'''
    graphics_insert = '''// Create screenshot from current OpenGL surface.
int vw_Screenshot(int Width, int Height, const std::string &FileName);
#ifdef AMIGACHROME
// Capture the composed default framebuffer immediately before the next swap.
void vw_ArmDiagnosticFrameCapture(const char *FileName,
                                  const char *SuccessMarkerName,
                                  const char *FailureMarkerName);
#endif
'''
    if graphics_anchor not in graphics_text:
        raise RuntimeError("AstroMenace screenshot declaration marker missing")
    graphics_text = graphics_text.replace(graphics_anchor, graphics_insert, 1)
    graphics_h.write_text(graphics_text, encoding="utf-8")

gl_text = gl_main_cpp.read_text(encoding="utf-8")
if "DiagnosticCaptureFile" not in gl_text:
    gl_include_anchor = '#include "SDL2/SDL.h"\n#include <string.h>\n'
    gl_include_insert = '''#include "SDL2/SDL.h"
#include <string.h>
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
    if gl_include_anchor not in gl_text:
        raise RuntimeError("AstroMenace gl_main include marker missing")
    gl_text = gl_text.replace(gl_include_anchor, gl_include_insert, 1)

    state_anchor = '''// resolve FBO (for blit main FBO with multisample)
std::shared_ptr<sFBO> ResolveFBO{};

} // unnamed namespace
'''
    state_insert = '''// resolve FBO (for blit main FBO with multisample)
std::shared_ptr<sFBO> ResolveFBO{};

#ifdef AMIGACHROME
const char *DiagnosticCaptureFile{nullptr};
const char *DiagnosticCaptureSuccessMarker{nullptr};
const char *DiagnosticCaptureFailureMarker{nullptr};

static void WriteDiagnosticCaptureMarker(const char *Name)
{
    if (!Name) return;
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}
#endif

} // unnamed namespace
'''
    if state_anchor not in gl_text:
        raise RuntimeError("AstroMenace gl_main FBO state marker missing")
    gl_text = gl_text.replace(state_anchor, state_insert, 1)

    function_anchor = '''/*
 * Get SDL window handle.
 */
uintptr_t  vw_GetSDLWindow()
'''
    function_insert = '''#ifdef AMIGACHROME
void vw_ArmDiagnosticFrameCapture(const char *FileName,
                                  const char *SuccessMarkerName,
                                  const char *FailureMarkerName)
{
    DiagnosticCaptureFile = FileName;
    DiagnosticCaptureSuccessMarker = SuccessMarkerName;
    DiagnosticCaptureFailureMarker = FailureMarkerName;
}
#endif

/*
 * Get SDL window handle.
 */
uintptr_t  vw_GetSDLWindow()
'''
    if function_anchor not in gl_text:
        raise RuntimeError("AstroMenace gl_main window-handle marker missing")
    gl_text = gl_text.replace(function_anchor, function_insert, 1)

    end_anchor = '''    }

    assert(SDLWindow);

    SDL_GL_SwapWindow(SDLWindow);
'''
    end_insert = '''    }

#ifdef AMIGACHROME
    // The main FBO has now been resolved and drawn to framebuffer 0. This is
    // the first point where glReadPixels sees the same composed image that the
    // following swap will present.
    if (DiagnosticCaptureFile) {
        int Width{0};
        int Height{0};
        SDL_GetWindowSize(SDLWindow, &Width, &Height);
        if (Width > 0 && Height > 0 &&
            vw_Screenshot(Width, Height, DiagnosticCaptureFile) == 0) {
            WriteDiagnosticCaptureMarker(DiagnosticCaptureSuccessMarker);
        } else {
            WriteDiagnosticCaptureMarker(DiagnosticCaptureFailureMarker);
        }
        DiagnosticCaptureFile = nullptr;
        DiagnosticCaptureSuccessMarker = nullptr;
        DiagnosticCaptureFailureMarker = nullptr;
    }
#endif

    assert(SDLWindow);

    SDL_GL_SwapWindow(SDLWindow);
'''
    if end_anchor not in gl_text:
        raise RuntimeError("AstroMenace pre-swap marker missing")
    gl_text = gl_text.replace(end_anchor, end_insert, 1)
    gl_main_cpp.write_text(gl_text, encoding="utf-8")

print("patched AstroMenace pre-swap composed-frame capture")
