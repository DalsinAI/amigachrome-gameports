#!/usr/bin/env python3
"""Add a diagnostic-only first-mission gate to AstroMenace on AmigaOS.

When PROGDIR:AUTOTEST_FIRST_MISSION exists, the target presents one real main
menu frame, requests the normal SWITCH_FROM_MENU_TO_GAME command, runs the
ordinary InitGame path, draws repeated game frames, captures a later mission
frame, and leaves guest-owned stage files. Without the sentinel file the game
behaves normally.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
loop_cpp = source / "src" / "loop_proc.cpp"
command_cpp = source / "src" / "command.cpp"

loop_text = loop_cpp.read_text(encoding="utf-8")
if "stage-first-game-frame-presented" not in loop_text:
    helper_anchor = '''static void AMFirstFrameStage(const char *Name)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}
'''
    helper_insert = helper_anchor + '''
static bool AMFirstMissionRequested()
{
    BPTR File = Open((CONST_STRPTR)"PROGDIR:AUTOTEST_FIRST_MISSION", MODE_OLDFILE);
    if (!File) return false;
    Close(File);
    return true;
}
'''
    if helper_anchor not in loop_text:
        raise RuntimeError("AstroMenace first-frame helper marker missing")
    loop_text = loop_text.replace(helper_anchor, helper_insert, 1)

    locals_anchor = '''    static bool FirstFrameProbe = true;
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-loop-enter");
'''
    locals_insert = '''    static bool FirstFrameProbe = true;
    static bool MissionAutotestRequested = false;
    static unsigned MissionFrameCount = 0;
    static bool MissionFrameCaptureAttempted = false;
    static bool MissionFramePresented = false;
    if (FirstFrameProbe) AMFirstFrameStage("PROGDIR:stage-loop-enter");
'''
    if locals_anchor not in loop_text:
        raise RuntimeError("AstroMenace first-frame local-state marker missing")
    loop_text = loop_text.replace(locals_anchor, locals_insert, 1)

    game_anchor = '''    case eMenuStatus::GAME:
        DrawGame();
        break;
'''
    game_insert = '''    case eMenuStatus::GAME:
#ifdef AMIGACHROME
        if (MissionFrameCount == 0) {
            AMFirstFrameStage("PROGDIR:stage-first-game-enter");
        }
#endif
        DrawGame();
#ifdef AMIGACHROME
        ++MissionFrameCount;
        if (MissionFrameCount == 1) {
            AMFirstFrameStage("PROGDIR:stage-first-game-drawn");
        }
#endif
        break;
'''
    if game_anchor not in loop_text:
        raise RuntimeError("AstroMenace game draw marker missing")
    loop_text = loop_text.replace(game_anchor, game_insert, 1)

    end2d_anchor = '''    vw_End2DMode();
#ifdef AMIGACHROME
    if (FirstFrameProbe) {
'''
    end2d_insert = '''    vw_End2DMode();
#ifdef AMIGACHROME
    if (MenuStatus == eMenuStatus::GAME &&
        MissionFrameCount >= 20 &&
        !MissionFrameCaptureAttempted) {
        MissionFrameCaptureAttempted = true;
        AMFirstFrameStage("PROGDIR:stage-first-game-frame-ready");
        vw_ArmDiagnosticFrameCapture(
            "PROGDIR:AstroMenace-first-mission.bmp",
            "PROGDIR:stage-first-game-frame-captured",
            "PROGDIR:stage-first-game-frame-capture-failed");
    }
    if (FirstFrameProbe) {
'''
    if end2d_anchor not in loop_text:
        raise RuntimeError("AstroMenace pre-swap probe marker missing")
    loop_text = loop_text.replace(end2d_anchor, end2d_insert, 1)

    presented_anchor = '''    if (FirstFrameProbe) {
        AMFirstFrameStage("PROGDIR:stage-first-frame-presented");
        FirstFrameProbe = false;
    }
#endif

    if (vw_GetKeyStatus(SDLK_ESCAPE)) {
'''
    presented_insert = '''    if (FirstFrameProbe) {
        AMFirstFrameStage("PROGDIR:stage-first-frame-presented");
        FirstFrameProbe = false;
    }
    if (MenuStatus == eMenuStatus::GAME &&
        MissionFrameCaptureAttempted &&
        !MissionFramePresented) {
        AMFirstFrameStage("PROGDIR:stage-first-game-frame-presented");
        MissionFramePresented = true;
    }
    if (!MissionAutotestRequested &&
        !FirstFrameProbe &&
        MenuStatus == eMenuStatus::MAIN_MENU &&
        AMFirstMissionRequested()) {
        AMFirstFrameStage("PROGDIR:stage-first-mission-requested");
        cCommand::GetInstance().Set(eCommand::SWITCH_FROM_MENU_TO_GAME);
        MissionAutotestRequested = true;
    }
#endif

    if (vw_GetKeyStatus(SDLK_ESCAPE)) {
'''
    if presented_anchor not in loop_text:
        raise RuntimeError("AstroMenace post-swap probe marker missing")
    loop_text = loop_text.replace(presented_anchor, presented_insert, 1)

    loop_cpp.write_text(loop_text, encoding="utf-8")

command_text = command_cpp.read_text(encoding="utf-8")
if "stage-game-init-complete" not in command_text:
    include_anchor = '#include "game.h" // FIXME "game.h" should be replaced by individual headers\n'
    include_insert = '''#include "game.h" // FIXME "game.h" should be replaced by individual headers
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
    if include_anchor not in command_text:
        raise RuntimeError("AstroMenace command include marker missing")
    command_text = command_text.replace(include_anchor, include_insert, 1)

    namespace_anchor = '''namespace viewizard {
namespace astromenace {

'''
    namespace_insert = '''namespace viewizard {
namespace astromenace {

#ifdef AMIGACHROME
static void AMCommandStage(const char *Name)
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
    if namespace_anchor not in command_text:
        raise RuntimeError("AstroMenace command namespace marker missing")
    command_text = command_text.replace(namespace_anchor, namespace_insert, 1)

    case_anchor = '''    case eCommand::SWITCH_FROM_MENU_TO_GAME: // also used for mission restart
        PrepareToSwitchStatus();
        InitGame();
        PlayMusicTheme(eMusicTheme::GAME, 2000, 2000);
        PlayVoicePhrase(eVoicePhrase::PrepareForAction, 1.0f);
        break;
'''
    case_insert = '''    case eCommand::SWITCH_FROM_MENU_TO_GAME: // also used for mission restart
#ifdef AMIGACHROME
        AMCommandStage("PROGDIR:stage-before-game-prepare");
#endif
        PrepareToSwitchStatus();
#ifdef AMIGACHROME
        AMCommandStage("PROGDIR:stage-before-game-init");
#endif
        InitGame();
#ifdef AMIGACHROME
        AMCommandStage("PROGDIR:stage-game-init-complete");
#endif
        PlayMusicTheme(eMusicTheme::GAME, 2000, 2000);
        PlayVoicePhrase(eVoicePhrase::PrepareForAction, 1.0f);
#ifdef AMIGACHROME
        AMCommandStage("PROGDIR:stage-game-audio-requested");
#endif
        break;
'''
    if case_anchor not in command_text:
        raise RuntimeError("AstroMenace menu-to-game command marker missing")
    command_text = command_text.replace(case_anchor, case_insert, 1)

    command_cpp.write_text(command_text, encoding="utf-8")

print("patched AstroMenace gated first-mission runtime probe")
