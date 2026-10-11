#!/usr/bin/env python3
"""Add persistent Amiga-side stage markers to the prepared AssaultCube client.

The host's persisted screen can stay dark while the RTG application is active,
and stdout/clientlog output can be lost when the guest resets.  These tiny files
therefore record progress from the guest filesystem itself, including pre-main,
SDL/OpenGL setup, individual model loads, map connection, first presentation
and shutdown.
"""

from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_runtime_diagnostics.py ASSAULTCUBE_SOURCE_ROOT")

root = Path(sys.argv[1]).resolve()
source = root / "source" / "src" / "main.cpp"
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


helper_anchor = '#include "cube.h"\n'
helper = r'''#include "cube.h"
#include <stdio.h>

#ifdef __amigaos__
static void ACStage(const char *path)
{
    FILE *stage = fopen(path, "wb");
    if(stage)
    {
        fputs("ready\n", stage);
        fflush(stage);
        fclose(stage);
    }
}

__attribute__((constructor)) static void ACBeforeMainStage(void)
{
    ACStage("PROGDIR:stage-ac-00-pre-main");
}
#else
#define ACStage(path) ((void)0)
#endif
'''
replace_once(helper_anchor, helper, "stage helper")

replace_once(
    'void cleanup(char *msg)         // single program exit point;\n{\n',
    'void cleanup(char *msg)         // single program exit point;\n{\n'
    '    ACStage(msg ? "PROGDIR:stage-ac-91-cleanup-fatal-enter" : '
    '"PROGDIR:stage-ac-90-cleanup-normal-enter");\n',
    "cleanup entry",
)

replace_once(
    '    SDL_Quit();\n}\n\nVAR(resetcfg, 0, 0, 1);',
    '    SDL_Quit();\n'
    '    ACStage(msg ? "PROGDIR:stage-ac-93-cleanup-fatal-after-sdl" : '
    '"PROGDIR:stage-ac-92-cleanup-normal-after-sdl");\n'
    '}\n\nVAR(resetcfg, 0, 0, 1);',
    "cleanup return",
)

replace_once(
    'void quit()                     // normal exit\n{\n',
    'void quit()                     // normal exit\n{\n'
    '    ACStage("PROGDIR:stage-ac-80-quit-enter");\n',
    "quit entry",
)

replace_once(
    '    cleanup(NULL);\n    popscontext();',
    '    ACStage("PROGDIR:stage-ac-81-quit-before-cleanup");\n'
    '    cleanup(NULL);\n'
    '    ACStage("PROGDIR:stage-ac-82-quit-after-cleanup");\n'
    '    popscontext();',
    "quit cleanup",
)

replace_once(
    '    DELETEP(clientlogfile);\n    exit(EXIT_SUCCESS);\n}\nCOMMAND(quit, "");',
    '    DELETEP(clientlogfile);\n'
    '    ACStage("PROGDIR:stage-ac-83-quit-before-exit");\n'
    '    exit(EXIT_SUCCESS);\n'
    '}\nCOMMAND(quit, "");',
    "quit exit",
)

replace_once(
    'void fatal(const char *s, ...)    // failure exit\n{\n',
    'void fatal(const char *s, ...)    // failure exit\n{\n'
    '    ACStage("PROGDIR:stage-ac-99-fatal");\n',
    "fatal entry",
)

replace_once(
    'int main(int argc, char **argv)\n{\n',
    'int main(int argc, char **argv)\n{\n'
    '    ACStage("PROGDIR:stage-ac-01-enter-main");\n',
    "main entry",
)

replace_once(
    '    initclientlog();\n    if(quitdirectly) return EXIT_SUCCESS;',
    '    initclientlog();\n'
    '    ACStage("PROGDIR:stage-ac-02-clientlog-ready");\n'
    '    if(quitdirectly) return EXIT_SUCCESS;',
    "client log",
)

replace_once(
    '    if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|par)<0) fatal("Unable to initialize SDL");\n',
    '    if(SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|par)<0) fatal("Unable to initialize SDL");\n'
    '    ACStage("PROGDIR:stage-ac-03-sdl-ready");\n',
    "SDL init",
)

replace_once(
    '    setupscreen(useddepthbits, usedfsaa);\n\n    initlog("video: misc");',
    '    setupscreen(useddepthbits, usedfsaa);\n'
    '    ACStage("PROGDIR:stage-ac-04-screen-ready");\n\n'
    '    initlog("video: misc");',
    "screen setup",
)

replace_once(
    '    gl_init(screenw, screenh, useddepthbits, usedfsaa);\n\n    notexture = noworldtexture',
    '    gl_init(screenw, screenh, useddepthbits, usedfsaa);\n'
    '    ACStage("PROGDIR:stage-ac-05-gl-ready");\n\n'
    '    notexture = noworldtexture',
    "GL setup",
)

replace_once(
    '    if(!notexture) fatal("could not find core textures (hint: run AssaultCube from the parent of the bin directory)");\n',
    '    if(!notexture) fatal("could not find core textures (hint: run AssaultCube from the parent of the bin directory)");\n'
    '    ACStage("PROGDIR:stage-ac-06-core-texture-ready");\n',
    "core texture",
)

replace_once(
    '    preload_entmodels();\n\n    initlog("docs");',
    '    preload_entmodels();\n'
    '    ACStage("PROGDIR:stage-ac-07-models-ready");\n\n'
    '    initlog("docs");',
    "models",
)

replace_once(
    '    localconnect();\n\n    if(initscript) execute(initscript);',
    '    localconnect();\n'
    '    ACStage("PROGDIR:stage-ac-08-localconnect-ready");\n\n'
    '    if(initscript) execute(initscript);',
    "local connect",
)

replace_once(
    '    initlog("mainloop");\n\n    inputgrab(grabinput = true);',
    '    initlog("mainloop");\n'
    '    if(clientlogfile) clientlogfile->fflush();\n'
    '    ACStage("PROGDIR:stage-ac-09-mainloop-enter");\n\n'
    '    inputgrab(grabinput = true);',
    "main loop",
)

replace_once(
    '            if(frames>4) SDL_GL_SwapWindow(screen);\n',
    '            if(frames>4)\n'
    '            {\n'
    '                SDL_GL_SwapWindow(screen);\n'
    '                static bool acFirstFrameMarked = false;\n'
    '                if(!acFirstFrameMarked)\n'
    '                {\n'
    '                    ACStage("PROGDIR:stage-ac-10-first-frame-presented");\n'
    '                    acFirstFrameMarked = true;\n'
    '                }\n'
    '            }\n',
    "first presented frame",
)

source.write_text(text, encoding="utf-8")

# Model loading is currently the first unqualified region.  Trace both the
# preload loop and the loader's format probes without depending on buffered
# client logging.
model_source = root / "source" / "src" / "rendermodel.cpp"
if not model_source.is_file():
    raise RuntimeError(f"AssaultCube rendermodel.cpp missing: {model_source}")
model_text = model_source.read_text(encoding="utf-8")


def replace_model_once(old: str, new: str, label: str) -> None:
    global model_text
    if new in model_text:
        return
    count = model_text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}")
    model_text = model_text.replace(old, new, 1)


model_helper = r'''#include "cube.h"
#include <stdio.h>

#ifdef __amigaos__
static void ACModelTrace(const char *name, int index, const char *phase)
{
    FILE *current = fopen("PROGDIR:stage-ac-model-current.txt", "wb");
    if(current)
    {
        fprintf(current, "%d\t%s\t%s\n", index, name ? name : "<null>", phase);
        fflush(current);
        fclose(current);
    }

    char path[128];
    snprintf(path, sizeof(path), "PROGDIR:stage-ac-model-%02d-%s", index, phase);
    FILE *stage = fopen(path, "wb");
    if(stage)
    {
        fputs("ready\n", stage);
        fflush(stage);
        fclose(stage);
    }
}

static void ACLoadTrace(const char *name, const char *phase)
{
    FILE *current = fopen("PROGDIR:stage-ac-loadmodel-current.txt", "wb");
    if(current)
    {
        fprintf(current, "%s\t%s\n", name ? name : "<null>", phase);
        fflush(current);
        fclose(current);
    }
}
#else
#define ACModelTrace(name, index, phase) ((void)0)
#define ACLoadTrace(name, phase) ((void)0)
#endif
'''
replace_model_once('#include "cube.h"\n', model_helper, "model helper")

replace_model_once(
    '    if(mdlnotfound.access(name)) return NULL;   // already tried to find that earlier -> not available\n',
    '    ACLoadTrace(name, "resolved-name");\n'
    '    if(mdlnotfound.access(name)) return NULL;   // already tried to find that earlier -> not available\n',
    "resolved model name",
)

replace_model_once(
    '        m = new md2(name);                      // try md2\n'
    '        loadingmodel = m;\n'
    '        loopi(MMA_NUM) DELSTRING(loadingattributes.n[i]);\n'
    '        if(!m->load())                          // md2 didn\'t load\n',
    '        ACLoadTrace(name, "before-md2-new");\n'
    '        m = new md2(name);                      // try md2\n'
    '        loadingmodel = m;\n'
    '        loopi(MMA_NUM) DELSTRING(loadingattributes.n[i]);\n'
    '        ACLoadTrace(name, "before-md2-load");\n'
    '        if(!m->load())                          // md2 didn\'t load\n',
    "MD2 probe",
)

replace_model_once(
    '        {\n'
    '            delete m;\n'
    '            m = new md3(name);                  // try md3\n'
    '            loadingmodel = m;\n'
    '            if(!m->load())                      // md3 didn\'t load -> we don\'t have that model\n',
    '        {\n'
    '            ACLoadTrace(name, "md2-load-failed");\n'
    '            delete m;\n'
    '            ACLoadTrace(name, "before-md3-new");\n'
    '            m = new md3(name);                  // try md3\n'
    '            loadingmodel = m;\n'
    '            ACLoadTrace(name, "before-md3-load");\n'
    '            if(!m->load())                      // md3 didn\'t load -> we don\'t have that model\n',
    "MD3 probe",
)

replace_model_once(
    '            {\n'
    '                delete m;\n'
    '                m = loadingmodel = NULL;\n',
    '            {\n'
    '                ACLoadTrace(name, "md3-load-failed");\n'
    '                delete m;\n'
    '                m = loadingmodel = NULL;\n',
    "MD3 failure",
)

replace_model_once(
    '        popscontext();\n'
    '        if(loadingmodel && m)\n',
    '        ACLoadTrace(name, m ? "format-load-complete" : "format-load-missing");\n'
    '        popscontext();\n'
    '        if(loadingmodel && m)\n',
    "format load completion",
)

old_preload = r'''void preload_entmodels()
{
    extern const char *entmdlnames[];
    for(int i = 0; entmdlnames[i][0]; i++)
    {
        model *mdl = loadmodel(entmdlnames[i]);
        if(dynshadow && mdl) mdl->genshadows(8.0f, 2.0f);
    }
    static const char *bouncemdlnames[] = { "misc/gib01", "misc/gib02", "misc/gib03", "weapons/grenade/static" };
    loopi(sizeof(bouncemdlnames)/sizeof(bouncemdlnames[0]))
    {
        model *mdl = loadmodel(bouncemdlnames[i]);
        if(dynshadow && mdl) mdl->genshadows(8.0f, 2.0f);
    }
}
'''
new_preload = r'''void preload_entmodels()
{
    extern const char *entmdlnames[];
    for(int i = 0; entmdlnames[i][0]; i++)
    {
        ACModelTrace(entmdlnames[i], i, "before");
        model *mdl = loadmodel(entmdlnames[i]);
        ACModelTrace(entmdlnames[i], i, "loaded");
        if(dynshadow && mdl) mdl->genshadows(8.0f, 2.0f);
        ACModelTrace(entmdlnames[i], i, "shadowed");
    }
    static const char *bouncemdlnames[] = { "misc/gib01", "misc/gib02", "misc/gib03", "weapons/grenade/static" };
    loopi(sizeof(bouncemdlnames)/sizeof(bouncemdlnames[0]))
    {
        const int stageindex = i + 20;
        ACModelTrace(bouncemdlnames[i], stageindex, "before");
        model *mdl = loadmodel(bouncemdlnames[i]);
        ACModelTrace(bouncemdlnames[i], stageindex, "loaded");
        if(dynshadow && mdl) mdl->genshadows(8.0f, 2.0f);
        ACModelTrace(bouncemdlnames[i], stageindex, "shadowed");
    }
}
'''
replace_model_once(old_preload, new_preload, "entity model preload")

model_source.write_text(model_text, encoding="utf-8")
print(f"patched AssaultCube runtime stage markers: {source}")
print(f"patched AssaultCube model-load stage markers: {model_source}")
