#!/usr/bin/env python3
"""Instrument prepared C-Dogs SDL sources with guest-owned stage markers."""

from pathlib import Path
import subprocess
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_runtime_diagnostics.py CDOGS_SOURCE_ROOT")

root = Path(sys.argv[1]).resolve()
subprocess.run(
    [sys.executable, str(Path(__file__).with_name("patch_amiga_paths.py")), str(root)],
    check=True,
)

source = root / "src" / "cdogs.c"
if not source.is_file():
    raise RuntimeError(f"C-Dogs entry source missing: {source}")

text = source.read_text(encoding="utf-8")


def replace_once(old: str, new: str, label: str) -> None:
    global text
    if new in text:
        return
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}")
    text = text.replace(old, new, 1)


anchor = '#include <string.h>\n'
helper = r'''#include <string.h>

#ifdef __amigaos__
static void CDogsStage(const char *path)
{
	FILE *stage = fopen(path, "wb");
	if (stage != NULL)
	{
		fputs("ready\n", stage);
		fflush(stage);
		fclose(stage);
	}
}

__attribute__((constructor)) static void CDogsBeforeMainStage(void)
{
	CDogsStage("PROGDIR:stage-cdogs-00-pre-main");
}
#else
#define CDogsStage(path) ((void)0)
#endif
'''
replace_once(anchor, helper, "stage helper")

replace_once(
    'int main(int argc, char *argv[])\n{\n',
    'int main(int argc, char *argv[])\n{\n'
    '\tsetvbuf(stdout, NULL, _IONBF, 0);\n'
    '\tsetvbuf(stderr, NULL, _IONBF, 0);\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-01-enter-main");\n',
    "main entry",
)

replace_once(
    '\tLogInit();\n\n\tPrintTitle();',
    '\tLogInit();\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-02-log-ready");\n\n'
    '\tPrintTitle();',
    "log init",
)

replace_once(
    '\tif (!ParseArgs(argc, argv, &connectAddr, &loadCampaign, &demoQuitTimer))\n'
    '\t{\n\t\tgoto bail;\n\t}\n',
    '\tif (!ParseArgs(argc, argv, &connectAddr, &loadCampaign, &demoQuitTimer))\n'
    '\t{\n\t\tgoto bail;\n\t}\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-03-args-ready");\n',
    "argument parse",
)

replace_once(
    '\tif (SDL_Init(sdlFlags) != 0)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Could not initialise SDL: %s", SDL_GetError());\n'
    '\t\terr = EXIT_FAILURE;\n'
    '\t\tgoto bail;\n'
    '\t}\n',
    '\tif (SDL_Init(sdlFlags) != 0)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Could not initialise SDL: %s", SDL_GetError());\n'
    '\t\terr = EXIT_FAILURE;\n'
    '\t\tgoto bail;\n'
    '\t}\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-04-sdl-ready");\n',
    "SDL init",
)

replace_once(
    '\tif (!gGraphicsDevice.IsInitialized)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Video didn\'t init!");\n'
    '\t\terr = EXIT_FAILURE;\n'
    '\t\tgoto bail;\n'
    '\t}\n'
    '\tFontLoadFromJSON',
    '\tif (!gGraphicsDevice.IsInitialized)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Video didn\'t init!");\n'
    '\t\terr = EXIT_FAILURE;\n'
    '\t\tgoto bail;\n'
    '\t}\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-05-video-ready");\n'
    '\tFontLoadFromJSON',
    "video init",
)

replace_once(
    '\tPicManagerLoad(&gPicManager);\n',
    '\tPicManagerLoad(&gPicManager);\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-06-graphics-loaded");\n',
    "graphics load",
)

replace_once(
    '\tif (!gSoundDevice.isInitialised)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Sound initialization failed!");\n'
    '\t}\n',
    '\tif (!gSoundDevice.isInitialised)\n'
    '\t{\n'
    '\t\tLOG(LM_MAIN, LL_ERROR, "Sound initialization failed!");\n'
    '\t}\n'
    '\tCDogsStage(gSoundDevice.isInitialised\n'
    '\t\t? "PROGDIR:stage-cdogs-07-audio-ready"\n'
    '\t\t: "PROGDIR:stage-cdogs-07-audio-unavailable");\n',
    "audio init",
)

replace_once(
    '\tLoadingScreenDraw(&gLoadingScreen, "Loading main menu...", 1.0f);\n',
    '\tCDogsStage("PROGDIR:stage-cdogs-08-data-ready");\n'
    '\tLoadingScreenDraw(&gLoadingScreen, "Loading main menu...", 1.0f);\n',
    "data ready",
)

replace_once(
    '\tLOG(LM_MAIN, LL_INFO, "Starting game");\n'
    '\tLoopRunnerRun(&l);\n'
    '\tLoopRunnerTerminate(&l);',
    '\tLOG(LM_MAIN, LL_INFO, "Starting game");\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-09-loop-enter");\n'
    '\tLoopRunnerRun(&l);\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-10-loop-returned");\n'
    '\tLoopRunnerTerminate(&l);',
    "main loop",
)

replace_once(
    'bail:\n\tNetServerTerminate(&gNetServer);',
    'bail:\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-80-cleanup-enter");\n'
    '\tNetServerTerminate(&gNetServer);',
    "cleanup entry",
)

replace_once(
    '\tSoundTerminate(&gSoundDevice, true);\n'
    '\tConfigDestroy(&gConfig);',
    '\tCDogsStage("PROGDIR:stage-cdogs-81-before-audio-close");\n'
    '\tSoundTerminate(&gSoundDevice, true);\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-82-after-audio-close");\n'
    '\tConfigDestroy(&gConfig);',
    "audio cleanup",
)

replace_once(
    '\tSDLJBN_Quit();\n'
    '\tSDL_Quit();\n\n'
    '\treturn err;',
    '\tSDLJBN_Quit();\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-83-before-sdl-quit");\n'
    '\tSDL_Quit();\n'
    '\tCDogsStage("PROGDIR:stage-cdogs-84-after-sdl-quit");\n\n'
    '\tCDogsStage(err == 0\n'
    '\t\t? "PROGDIR:stage-cdogs-90-return-zero"\n'
    '\t\t: "PROGDIR:stage-cdogs-99-return-error");\n'
    '\treturn err;',
    "SDL cleanup",
)

source.write_text(text, encoding="utf-8")
print(f"patched C-Dogs runtime stage markers: {source}")
