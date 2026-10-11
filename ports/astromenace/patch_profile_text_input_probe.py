#!/usr/bin/env python3
"""Add a qualification-only real profile text-entry journey.

With PROGDIR:AUTOTEST_PROFILE_INPUT present, the diagnostic build clears the
in-memory pilot list, opens AstroMenace's normal profile menu, waits for real
SDL_TEXTINPUT, records the entered text, creates and saves the pilot, then
leaves through the normal main-loop shutdown. A second launch with
PROGDIR:AUTOTEST_PROFILE_VERIFY proves the saved DALE profile is loaded.

The probe does not synthesize text inside the game. Input must arrive through
the AmigaOS SDL backend, which is the behaviour under qualification.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
loop_cpp = source / "src" / "loop_proc.cpp"
profile_cpp = source / "src" / "menu" / "menu_profile.cpp"


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one {label} marker, got {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


loop_text = loop_cpp.read_text(encoding="utf-8")
if "stage-profile-input-ready" not in loop_text:
    include_anchor = '#include "SDL2/SDL.h"\n'
    include_insert = '''#include "SDL2/SDL.h"
#ifdef AMIGACHROME
#include <string.h>
#endif
'''
    if include_anchor not in loop_text:
        raise RuntimeError("AstroMenace loop SDL include marker missing")
    loop_text = loop_text.replace(include_anchor, include_insert, 1)

    locals_anchor = '''    static bool KeyboardProbeQueued = false;
    static bool CleanExitRequested = false;
'''
    locals_insert = '''    static bool KeyboardProbeQueued = false;
    static bool CleanExitRequested = false;
    static bool ProfileInputPrepared = false;
    static bool ProfileMenuRequested = false;
    static bool ProfileInputReady = false;
    static bool ProfileVerifyHandled = false;
'''
    if locals_anchor not in loop_text:
        raise RuntimeError("AstroMenace input-exit probe locals marker missing")
    loop_text = loop_text.replace(locals_anchor, locals_insert, 1)

    journey_anchor = '''    if (!MissionAutotestRequested &&
        !FirstFrameProbe &&
        MenuStatus == eMenuStatus::MAIN_MENU &&
        AMFirstMissionRequested()) {
'''
    journey_insert = '''    if (!ProfileInputPrepared &&
        AMFirstMissionStageExists("PROGDIR:AUTOTEST_PROFILE_INPUT")) {
        InitDialogBoxes();
        CurrentProfile = -1;
        ChangeGameConfig().LastProfile = -1;
        NewProfileName.clear();
        for (unsigned i = 0; i < config::MAX_PROFILES; ++i) {
            ChangeGameConfig().Profile[i] = sPilotProfile{};
        }
        ChangeGameConfig().NeedShowHint[0] = false;
        AMFirstFrameStage("PROGDIR:stage-profile-state-cleared");
        ProfileInputPrepared = true;
    }
    if (ProfileInputPrepared &&
        !ProfileMenuRequested &&
        !FirstFrameProbe &&
        MenuStatus == eMenuStatus::MAIN_MENU) {
        SetMenu(eMenuStatus::PROFILE);
        AMFirstFrameStage("PROGDIR:stage-profile-menu-requested");
        ProfileMenuRequested = true;
    }
    if (ProfileMenuRequested &&
        !ProfileInputReady &&
        MenuStatus == eMenuStatus::PROFILE &&
        MenuContentTransp >= 0.99f &&
        !isDialogBoxDrawing()) {
        AMFirstFrameStage("PROGDIR:stage-profile-input-ready");
        ProfileInputReady = true;
    }
    if (!ProfileVerifyHandled &&
        AMFirstMissionStageExists("PROGDIR:AUTOTEST_PROFILE_VERIFY") &&
        !FirstFrameProbe &&
        MenuStatus == eMenuStatus::MAIN_MENU) {
        if (CurrentProfile >= 0 &&
            CurrentProfile < static_cast<int>(config::MAX_PROFILES) &&
            GameConfig().Profile[CurrentProfile].Used &&
            strcmp(GameConfig().Profile[CurrentProfile].Name, "DALE") == 0) {
            AMFirstFrameStage("PROGDIR:stage-profile-reloaded");
        } else {
            AMFirstFrameStage("PROGDIR:stage-profile-reload-failed");
        }
        QuitFromMainLoop();
        ProfileVerifyHandled = true;
    }
    if (!MissionAutotestRequested &&
        !FirstFrameProbe &&
        MenuStatus == eMenuStatus::MAIN_MENU &&
        AMFirstMissionRequested()) {
'''
    if journey_anchor not in loop_text:
        raise RuntimeError("AstroMenace mission journey request marker missing")
    loop_text = loop_text.replace(journey_anchor, journey_insert, 1)
    loop_cpp.write_text(loop_text, encoding="utf-8")

profile_text = profile_cpp.read_text(encoding="utf-8")
if "stage-profile-created" not in profile_text:
    include_anchor = '#include "SDL2/SDL.h"\n'
    include_insert = '''#include "SDL2/SDL.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
    if include_anchor not in profile_text:
        raise RuntimeError("AstroMenace profile SDL include marker missing")
    profile_text = profile_text.replace(include_anchor, include_insert, 1)

    namespace_anchor = '''namespace viewizard {
namespace astromenace {

'''
    namespace_insert = '''namespace viewizard {
namespace astromenace {

#ifdef AMIGACHROME
void QuitFromMainLoop();

static bool AMProfileInputRequested()
{
    BPTR File = Open((CONST_STRPTR)"PROGDIR:AUTOTEST_PROFILE_INPUT", MODE_OLDFILE);
    if (!File) return false;
    Close(File);
    return true;
}

static void AMProfileStage(const char *Name)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}

static void AMProfileWrite(const char *Name, const char *Text)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        if (Text) Write(File, (APTR)Text, strlen(Text));
        static const char Newline[] = "\\n";
        Write(File, (APTR)Newline, sizeof(Newline) - 1);
        Close(File);
    }
}
#endif

'''
    if namespace_anchor not in profile_text:
        raise RuntimeError("AstroMenace profile namespace marker missing")
    profile_text = profile_text.replace(namespace_anchor, namespace_insert, 1)

    new_record_anchor = '''    ChangeGameConfig().Profile[CurrentProfile].LastMission = CurrentMission;
    ProfileDifficulty(ProfileNum, eDifficultyAction::Update);
}
'''
    new_record_insert = '''    ChangeGameConfig().Profile[CurrentProfile].LastMission = CurrentMission;
    ProfileDifficulty(ProfileNum, eDifficultyAction::Update);
#ifdef AMIGACHROME
    if (AMProfileInputRequested()) {
        AMProfileWrite("PROGDIR:profile-created.txt",
                       GameConfig().Profile[CurrentProfile].Name);
        AMProfileStage("PROGDIR:stage-profile-created");
        SaveXMLConfigFile();
        AMProfileStage("PROGDIR:stage-profile-saved");
        QuitFromMainLoop();
    }
#endif
}
'''
    if new_record_anchor not in profile_text:
        raise RuntimeError("AstroMenace new-profile completion marker missing")
    profile_text = profile_text.replace(new_record_anchor, new_record_insert, 1)

    text_anchor = '''            NewProfileName += vw_GetCurrentUnicodeChar();

            if (vw_IsSoundAvailable(SoundTaping)) {
'''
    text_insert = '''            NewProfileName += vw_GetCurrentUnicodeChar();
#ifdef AMIGACHROME
            if (AMProfileInputRequested()) {
                std::string ProbeText = ConvertUTF8.to_bytes(NewProfileName);
                AMProfileWrite("PROGDIR:profile-text.txt", ProbeText.c_str());
                AMProfileStage("PROGDIR:stage-profile-text-received");
            }
#endif

            if (vw_IsSoundAvailable(SoundTaping)) {
'''
    if text_anchor not in profile_text:
        raise RuntimeError("AstroMenace profile Unicode append marker missing")
    profile_text = profile_text.replace(text_anchor, text_insert, 1)
    profile_cpp.write_text(profile_text, encoding="utf-8")

print("patched AstroMenace real SDL text-input profile qualification")
