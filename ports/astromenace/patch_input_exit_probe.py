#!/usr/bin/env python3
"""Add qualification-only keyboard, shutdown and clean-exit milestones.

After the first playable mission frame is presented, and only when
PROGDIR:AUTOTEST_FIRST_MISSION is present, the diagnostic target queues an
SDL key-down/key-up pair. The ordinary AstroMenace event loop must
observe both events before the probe requests Loop() to end. Every subsequent
cleanup operation is bracketed with stage files so a shutdown stall is located
precisely rather than being mistaken for a launch or rendering failure.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
loop_cpp = source / "src" / "loop_proc.cpp"
main_cpp = source / "src" / "main.cpp"

loop_text = loop_cpp.read_text(encoding="utf-8")
if "stage-keyboard-events-queued" not in loop_text:
    helper_anchor = '''static bool AMFirstMissionRequested()
{
    BPTR File = Open((CONST_STRPTR)"PROGDIR:AUTOTEST_FIRST_MISSION", MODE_OLDFILE);
    if (!File) return false;
    Close(File);
    return true;
}
'''
    helper_insert = helper_anchor + '''
static bool AMFirstMissionStageExists(const char *Name)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_OLDFILE);
    if (!File) return false;
    Close(File);
    return true;
}
'''
    if helper_anchor not in loop_text:
        raise RuntimeError("AstroMenace mission sentinel helper marker missing")
    loop_text = loop_text.replace(helper_anchor, helper_insert, 1)

    declaration_anchor = '''/*
 * Main loop.
 */
void Loop_Proc()
'''
    declaration_insert = '''#ifdef AMIGACHROME
// Defined in main.cpp; the runtime probe uses the game's ordinary loop exit.
void QuitFromMainLoop();
#endif

/*
 * Main loop.
 */
void Loop_Proc()
'''
    if "void QuitFromMainLoop();" not in loop_text:
        if declaration_anchor not in loop_text:
            raise RuntimeError("AstroMenace main-loop declaration marker missing")
        loop_text = loop_text.replace(declaration_anchor, declaration_insert, 1)

    locals_anchor = '''    static bool MissionFrameCaptureAttempted = false;
    static bool MissionFramePresented = false;
'''
    locals_insert = '''    static bool MissionFrameCaptureAttempted = false;
    static bool MissionFramePresented = false;
    static bool KeyboardProbeQueued = false;
    static bool CleanExitRequested = false;
'''
    if locals_anchor not in loop_text:
        raise RuntimeError("AstroMenace mission probe local-state marker missing")
    loop_text = loop_text.replace(locals_anchor, locals_insert, 1)

    presented_anchor = '''    if (MenuStatus == eMenuStatus::GAME &&
        MissionFrameCaptureAttempted &&
        !MissionFramePresented) {
        AMFirstFrameStage("PROGDIR:stage-first-game-frame-presented");
        MissionFramePresented = true;
    }
    if (!MissionAutotestRequested &&
'''
    presented_insert = '''    if (MenuStatus == eMenuStatus::GAME &&
        MissionFrameCaptureAttempted &&
        !MissionFramePresented) {
        AMFirstFrameStage("PROGDIR:stage-first-game-frame-presented");
        MissionFramePresented = true;
    }
    // Only a qualification run (PROGDIR:AUTOTEST_FIRST_MISSION present)
    // presses F11 and ends the game; a player keeps playing.
    if (MissionFramePresented && !KeyboardProbeQueued &&
        AMFirstMissionRequested()) {
        SDL_Event KeyDown{};
        KeyDown.type = SDL_KEYDOWN;
        KeyDown.key.type = SDL_KEYDOWN;
        KeyDown.key.state = SDL_PRESSED;
        KeyDown.key.keysym.sym = SDLK_F11;
        SDL_Event KeyUp{};
        KeyUp.type = SDL_KEYUP;
        KeyUp.key.type = SDL_KEYUP;
        KeyUp.key.state = SDL_RELEASED;
        KeyUp.key.keysym.sym = SDLK_F11;
        if (SDL_PushEvent(&KeyDown) == 1 && SDL_PushEvent(&KeyUp) == 1) {
            AMFirstFrameStage("PROGDIR:stage-keyboard-events-queued");
            KeyboardProbeQueued = true;
        } else {
            AMFirstFrameStage("PROGDIR:stage-keyboard-events-queue-failed");
        }
    }
    if (KeyboardProbeQueued &&
        !CleanExitRequested &&
        AMFirstMissionStageExists("PROGDIR:stage-keydown-received") &&
        AMFirstMissionStageExists("PROGDIR:stage-keyup-received") &&
        AMFirstMissionStageExists("PROGDIR:stage-audio-channel-active")) {
        AMFirstFrameStage("PROGDIR:stage-clean-exit-requested");
        QuitFromMainLoop();
        CleanExitRequested = true;
    }
    if (!MissionAutotestRequested &&
'''
    if presented_anchor not in loop_text:
        raise RuntimeError("AstroMenace mission-presented marker missing")
    loop_text = loop_text.replace(presented_anchor, presented_insert, 1)
    loop_cpp.write_text(loop_text, encoding="utf-8")

main_text = main_cpp.read_text(encoding="utf-8")
if "stage-keydown-received" not in main_text:
    namespace_anchor = '''namespace {

bool NeedQuitFromLoop{false};
bool NeedRecreateWindow{false};

} // unnamed namespace
'''
    namespace_insert = '''namespace {

bool NeedQuitFromLoop{false};
bool NeedRecreateWindow{false};

#ifdef AMIGACHROME
static void AMInputExitStage(const char *Name)
{
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
    if namespace_anchor not in main_text:
        raise RuntimeError("AstroMenace main anonymous namespace marker missing")
    main_text = main_text.replace(namespace_anchor, namespace_insert, 1)

    key_anchor = '''            case SDL_KEYDOWN:
                vw_KeyStatusUpdate(event.key.keysym.sym, true);
                break;

            case SDL_KEYUP:
                vw_KeyStatusUpdate(event.key.keysym.sym, false);
                break;
'''
    key_insert = '''            case SDL_KEYDOWN:
#ifdef AMIGACHROME
                if (event.key.keysym.sym == SDLK_F11) {
                    AMInputExitStage("PROGDIR:stage-keydown-received");
                }
#endif
                vw_KeyStatusUpdate(event.key.keysym.sym, true);
                break;

            case SDL_KEYUP:
#ifdef AMIGACHROME
                if (event.key.keysym.sym == SDLK_F11) {
                    AMInputExitStage("PROGDIR:stage-keyup-received");
                }
#endif
                vw_KeyStatusUpdate(event.key.keysym.sym, false);
                break;
'''
    if key_anchor not in main_text:
        raise RuntimeError("AstroMenace SDL keyboard-event marker missing")
    main_text = main_text.replace(key_anchor, key_insert, 1)

    loop_anchor = '''    // Main loop.
    Loop();

    CursorRelease();
'''
    loop_insert = '''    // Main loop.
    Loop();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-loop-exited");
#endif

    CursorRelease();
'''
    if loop_anchor not in main_text:
        raise RuntimeError("AstroMenace main-loop return marker missing")
    main_text = main_text.replace(loop_anchor, loop_insert, 1)

    cleanup_anchor = '''    CursorRelease();
    DestroyInfoObject();
    WorkshopDestroyData();

    ReleaseAllObject3D();
    vw_ReleaseAllModel3D();
    vw_ReleaseAllParticleSystems2D();
    vw_ReleaseAllParticleSystems();
    vw_ReleaseAllLights();

    vw_ReleaseAllFontChars();
    vw_ReleaseAllTextures();
    ShadowMap_Release();

    // gl stuff -> gl context -> window
    vw_ReleaseOpenGLStuff();
    vw_DeleteOpenGLContext();
    vw_DestroyWindow();

    // important, save config before game's window recreate
    SaveXMLConfigFile();
'''
    cleanup_insert = '''#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-before-cursor-release");
#endif
    CursorRelease();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-cursor-release");
    AMInputExitStage("PROGDIR:stage-before-info-destroy");
#endif
    DestroyInfoObject();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-info-destroy");
    AMInputExitStage("PROGDIR:stage-before-workshop-destroy");
#endif
    WorkshopDestroyData();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-workshop-destroy");
    AMInputExitStage("PROGDIR:stage-before-object-release");
#endif

    ReleaseAllObject3D();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-object-release");
    AMInputExitStage("PROGDIR:stage-before-model-release");
#endif
    vw_ReleaseAllModel3D();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-model-release");
    AMInputExitStage("PROGDIR:stage-before-particles2d-release");
#endif
    vw_ReleaseAllParticleSystems2D();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-particles2d-release");
    AMInputExitStage("PROGDIR:stage-before-particles-release");
#endif
    vw_ReleaseAllParticleSystems();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-particles-release");
    AMInputExitStage("PROGDIR:stage-before-lights-release");
#endif
    vw_ReleaseAllLights();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-lights-release");
    AMInputExitStage("PROGDIR:stage-before-fontchars-release");
#endif

    vw_ReleaseAllFontChars();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-fontchars-release");
    AMInputExitStage("PROGDIR:stage-before-textures-release");
#endif
    vw_ReleaseAllTextures();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-textures-release");
    AMInputExitStage("PROGDIR:stage-before-shadowmap-release");
#endif
    ShadowMap_Release();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-shadowmap-release");
    AMInputExitStage("PROGDIR:stage-before-glstuff-release");
#endif

    // gl stuff -> gl context -> window
    vw_ReleaseOpenGLStuff();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-glstuff-release");
    AMInputExitStage("PROGDIR:stage-before-glcontext-delete");
#endif
    vw_DeleteOpenGLContext();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-glcontext-delete");
    AMInputExitStage("PROGDIR:stage-before-window-destroy");
#endif
    vw_DestroyWindow();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-window-destroy");
    AMInputExitStage("PROGDIR:stage-before-config-save");
#endif

    // important, save config before game's window recreate
    SaveXMLConfigFile();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-config-save");
#endif
'''
    position = main_text.rfind(cleanup_anchor)
    if position < 0:
        raise RuntimeError("AstroMenace principal cleanup block marker missing")
    main_text = (
        main_text[:position]
        + cleanup_insert
        + main_text[position + len(cleanup_anchor):]
    )

    shutdown_anchor = '''    vw_ShutdownFont();
    vw_ReleaseText();
    vw_ShutdownAudio();
    vw_ShutdownVFS();
    JoystickClose();
    vw_ReleaseAllTimeThread();
    SDL_Quit();
    return 0;
}
'''
    shutdown_insert = '''#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-before-font-shutdown");
#endif
    vw_ShutdownFont();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-font-shutdown");
    AMInputExitStage("PROGDIR:stage-before-text-release");
#endif
    vw_ReleaseText();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-text-release");
    AMInputExitStage("PROGDIR:stage-before-audio-shutdown");
#endif
    vw_ShutdownAudio();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-audio-shutdown");
    AMInputExitStage("PROGDIR:stage-before-vfs-shutdown");
#endif
    vw_ShutdownVFS();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-vfs-shutdown");
    AMInputExitStage("PROGDIR:stage-before-joystick-close");
#endif
    JoystickClose();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-joystick-close");
    AMInputExitStage("PROGDIR:stage-before-time-release");
#endif
    vw_ReleaseAllTimeThread();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-time-release");
    AMInputExitStage("PROGDIR:stage-before-sdl-quit");
#endif
    SDL_Quit();
#ifdef AMIGACHROME
    AMInputExitStage("PROGDIR:stage-after-sdl-quit");
    AMInputExitStage("PROGDIR:stage-shutdown-complete");
#endif
    return 0;
}
'''
    position = main_text.rfind(shutdown_anchor)
    if position < 0:
        raise RuntimeError("AstroMenace final shutdown marker missing")
    main_text = (
        main_text[:position]
        + shutdown_insert
        + main_text[position + len(shutdown_anchor):]
    )
    main_cpp.write_text(main_text, encoding="utf-8")

print("patched AstroMenace keyboard, shutdown trace and clean-exit qualification")
