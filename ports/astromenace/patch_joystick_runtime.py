#!/usr/bin/env python3
"""Restore AstroMenace joystick startup after the first-light bring-up.

The initial AC090 port deliberately deferred all joystick calls while SDL,
VirGL, VFS and asset loading were being isolated. Those paths are now stable.
Initialize SDL's joystick subsystem as a non-fatal optional service and then
run AstroMenace's normal joystick discovery. Machines without a controller
continue normally and hot-plug events remain available to the main loop.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
main_cpp = source / "src" / "main.cpp"
text = main_cpp.read_text(encoding="utf-8")

old = '''    // Amiga first-light: joystick intentionally deferred until runtime is stable.
#ifdef AMIGACHROME
    AMDiag("joystick deferred");
#else
    JoystickInit(vw_GetTimeThread(0));
#endif
'''
new = '''    // The first-light joystick deferred phase is complete. Joystick remains
    // optional on AmigaChrome: initialise its SDL subsystem only after timer,
    // VFS, configuration and font setup are known-good. Failure must not
    // prevent keyboard or mouse play.
#ifdef AMIGACHROME
    AMDiag("before joystick subsystem init");
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
        std::cerr << __func__ << "(): Couldn't init SDL joystick: "
                  << SDL_GetError() << "\\n";
        AMDiag("joystick subsystem unavailable");
    } else {
        JoystickInit(vw_GetTimeThread(0));
        AMDiag("joystick subsystem ready");
    }
#else
    JoystickInit(vw_GetTimeThread(0));
#endif
'''

if "before joystick subsystem init" not in text:
    if old not in text:
        raise RuntimeError("AstroMenace deferred joystick marker missing")
    text = text.replace(old, new, 1)

main_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace optional joystick subsystem startup")
