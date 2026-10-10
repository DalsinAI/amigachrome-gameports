#!/usr/bin/env python3
"""Remove qualification-only guest artifacts from an AstroMenace release tree.

Startup console diagnostics remain useful in a field build, but release users
must not receive CI stage files or asset-loader trace spam beside the game.
The runtime qualification build skips this cleanup and retains those markers.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
main_cpp = source / "src" / "main.cpp"
loading_cpp = source / "src" / "assets" / "loading.cpp"

main_text = main_cpp.read_text(encoding="utf-8")
for block in (
    '''\n#ifdef AMIGACHROME
    AMStageFile("PROGDIR:stage-after-asset-load");
#endif''',
    '''\n#ifdef AMIGACHROME
    AMStageFile("PROGDIR:stage-main-menu-ready");
#endif''',
):
    main_text = main_text.replace(block, "")

stage_helper = '''
static void AMStageFile(const char *Name)
{
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}
'''
main_text = main_text.replace(stage_helper, "")
if "stage-after-asset-load" in main_text or "stage-main-menu-ready" in main_text:
    raise RuntimeError("AstroMenace release stage-file cleanup was incomplete")
main_cpp.write_text(main_text, encoding="utf-8")

loading_text = loading_cpp.read_text(encoding="utf-8")
helper = '''#include "SDL2/SDL.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
static void AMAssetDiag(const char *Message)
{
    BPTR Out = Output();
    if (Out) {
        FPuts(Out, (CONST_STRPTR)"AMLOAD: ");
        FPuts(Out, (CONST_STRPTR)Message);
        FPuts(Out, (CONST_STRPTR)"\\n");
    }
}
#else
static void AMAssetDiag(const char *) {}
#endif
'''
if helper in loading_text:
    loading_text = loading_text.replace(helper, '#include "SDL2/SDL.h"\n', 1)

for message in (
    "before audio assets",
    "after audio assets",
    "before model assets",
    "after model assets",
    "before texture assets",
    "after texture assets",
    "before progress texture release",
    "after progress bar release",
    "after progress border release",
    "asset loader returning",
):
    loading_text = loading_text.replace(f'    AMAssetDiag("{message}");\n', "")

if "AMAssetDiag(" in loading_text or "AMLOAD:" in loading_text:
    raise RuntimeError("AstroMenace release asset-trace cleanup was incomplete")
loading_cpp.write_text(loading_text, encoding="utf-8")

print("removed AstroMenace qualification stage files and asset traces")
