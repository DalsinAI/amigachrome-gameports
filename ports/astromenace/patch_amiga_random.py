#!/usr/bin/env python3
"""Fix and diagnose AstroMenace startup/runtime on AmigaOS.

The upstream GCC 16 fix moved the random engine to a function-local static,
but it still seeds that engine with std::random_device on first use.  The
classic AmigaOS libstdc++ build has no reliable random_device entropy backend;
that first use can happen from a global constructor and terminate before
main().  Use a safe non-zero seed on the Amiga target and reseed once SDL's
timer subsystem is alive.

The target also installs explicit new/terminate diagnostics and traces each
texture preload with free/largest memory figures.  This keeps the next failure
boundary visible even when the C++ runtime can only report "Program aborted".
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()

rand_cpp = source / "src" / "core" / "math" / "rand.cpp"
math_h = source / "src" / "core" / "math" / "math.h"
main_cpp = source / "src" / "main.cpp"
texture_cpp = source / "src" / "assets" / "texture.cpp"

rand_text = rand_cpp.read_text(encoding="utf-8")
old_seed = "    static std::default_random_engine gen{std::random_device{}()};"
new_seed = """#ifdef AMIGACHROME
    // AmigaOS libstdc++ has no dependable std::random_device entropy source.
    // A failure here occurs in a global constructor before main() and ends as
    // a bare \"Program aborted\".  Start from a valid deterministic state;
    // main() reseeds after SDL_INIT_TIMER is available.
    static std::default_random_engine gen{0x41535452u};
#else
    static std::default_random_engine gen{std::random_device{}()};
#endif"""

if "0x41535452u" not in rand_text:
    if old_seed not in rand_text:
        raise RuntimeError("AstroMenace random-engine seed marker missing")
    rand_text = rand_text.replace(old_seed, new_seed, 1)

seed_function = """
#ifdef AMIGACHROME
/*
 * Replace the conservative pre-main seed once SDL's timer subsystem is live.
 * Keep zero out of minstd_rand0's state because zero is its fixed point.
 */
void vw_SeedRandom(unsigned Seed)
{
    GetRandomEngine().seed(Seed ? Seed : 1u);
}
#endif

"""
rand_anchor = "/*\n * Generate random float in range [0.0f, 1.0f).\n */\nfloat vw_fRand()"
if "void vw_SeedRandom(unsigned Seed)" not in rand_text:
    if rand_anchor not in rand_text:
        raise RuntimeError("AstroMenace random function marker missing")
    rand_text = rand_text.replace(rand_anchor, seed_function + rand_anchor, 1)

rand_cpp.write_text(rand_text, encoding="utf-8")

math_text = math_h.read_text(encoding="utf-8")
math_anchor = "// Generate random float in range [0.0f, 1.0f).\nfloat vw_fRand();"
math_insert = """// Generate random float in range [0.0f, 1.0f).
float vw_fRand();
#ifdef AMIGACHROME
// Reseed the target-safe random engine after SDL's timer is available.
void vw_SeedRandom(unsigned Seed);
#endif"""
if "void vw_SeedRandom(unsigned Seed);" not in math_text:
    if math_anchor not in math_text:
        raise RuntimeError("AstroMenace math header random marker missing")
    math_text = math_text.replace(math_anchor, math_insert, 1)
math_h.write_text(math_text, encoding="utf-8")

main_text = main_cpp.read_text(encoding="utf-8")
main_anchor = '    AMDiag("after SDL base init");\n'
main_insert = '''    AMDiag("after SDL base init");
#ifdef AMIGACHROME
    vw_SeedRandom(static_cast<unsigned>(SDL_GetTicks()) ^ 0x41535452u);
    AMDiag("random engine reseeded");
#endif
'''
if 'AMDiag("random engine reseeded")' not in main_text:
    if main_anchor not in main_text:
        raise RuntimeError("AstroMenace post-SDL diagnostic marker missing")
    main_text = main_text.replace(main_anchor, main_insert, 1)

if "AMMEM:" not in main_text:
    include_anchor = "#include <dos/dos.h>\n"
    include_insert = """#include <dos/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>
#include <new>
#include <exception>
#include <cstdlib>
"""
    if include_anchor not in main_text:
        raise RuntimeError("AstroMenace Amiga diagnostic include marker missing")
    main_text = main_text.replace(include_anchor, include_insert, 1)

    function_anchor = "int main(int argc, char *argv[])\n{\n"
    diagnostic_functions = """#ifdef AMIGACHROME
static void AMMemory(const char *Where)
{
    Printf((CONST_STRPTR)\"AMMEM: %s free=%lu largest=%lu\\n\",
           Where,
           (ULONG)AvailMem(MEMF_ANY),
           (ULONG)AvailMem(MEMF_LARGEST));
}

static void AMOutOfMemory()
{
    AMMemory(\"new_handler\");
    AMDiag(\"out of memory\");
    std::abort();
}

static void AMTerminateFailure()
{
    AMMemory(\"terminate\");
    AMDiag(\"std::terminate\");
    std::abort();
}
#endif

int main(int argc, char *argv[])
{
"""
    if function_anchor not in main_text:
        raise RuntimeError("AstroMenace main function marker missing")
    main_text = main_text.replace(function_anchor, diagnostic_functions, 1)

    entry_anchor = '    AMDiag("entered main");\n'
    entry_insert = '''    AMDiag("entered main");
#ifdef AMIGACHROME
    std::set_new_handler(AMOutOfMemory);
    std::set_terminate(AMTerminateFailure);
    AMMemory("main start");
#endif
'''
    if entry_anchor not in main_text:
        raise RuntimeError("AstroMenace main-entry diagnostic marker missing")
    main_text = main_text.replace(entry_anchor, entry_insert, 1)

main_cpp.write_text(main_text, encoding="utf-8")

texture_text = texture_cpp.read_text(encoding="utf-8")
if "AMTEX: begin" not in texture_text:
    include_anchor = '#include "../config/config.h"\n'
    include_insert = '''#include "../config/config.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>
#endif
'''
    if include_anchor not in texture_text:
        raise RuntimeError("AstroMenace texture include marker missing")
    texture_text = texture_text.replace(include_anchor, include_insert, 1)

    old_loop = '''    for (auto &tmpAsset : TextureMap) {
        vw_SetTextureProp(sTextureFilter{tmpAsset.second.TextFilter},
                          tmpAsset.second.NeedAnisotropy ? GameConfig().AnisotropyLevel : 1,
                          sTextureWrap{tmpAsset.second.TextWrap}, tmpAsset.second.Alpha,
                          tmpAsset.second.AlphaMode, tmpAsset.second.MipMap);
        tmpAsset.second.PreloadedTexture = vw_LoadTexture(tmpAsset.second.TextureFile);
        function(TextureLoadValue);
    }
'''
    new_loop = '''#ifdef AMIGACHROME
    ULONG AmigaTextureIndex = 0;
#endif
    for (auto &tmpAsset : TextureMap) {
#ifdef AMIGACHROME
        ++AmigaTextureIndex;
        Printf((CONST_STRPTR)"AMTEX: begin %lu/%lu %s free=%lu largest=%lu\\n",
               AmigaTextureIndex,
               (ULONG)TextureMap.size(),
               tmpAsset.second.TextureFile.c_str(),
               (ULONG)AvailMem(MEMF_ANY),
               (ULONG)AvailMem(MEMF_LARGEST));
#endif
        vw_SetTextureProp(sTextureFilter{tmpAsset.second.TextFilter},
                          tmpAsset.second.NeedAnisotropy ? GameConfig().AnisotropyLevel : 1,
                          sTextureWrap{tmpAsset.second.TextWrap}, tmpAsset.second.Alpha,
                          tmpAsset.second.AlphaMode, tmpAsset.second.MipMap);
        tmpAsset.second.PreloadedTexture = vw_LoadTexture(tmpAsset.second.TextureFile);
#ifdef AMIGACHROME
        Printf((CONST_STRPTR)"AMTEX: loaded %lu/%lu %s id=%lu free=%lu largest=%lu\\n",
               AmigaTextureIndex,
               (ULONG)TextureMap.size(),
               tmpAsset.second.TextureFile.c_str(),
               (ULONG)tmpAsset.second.PreloadedTexture,
               (ULONG)AvailMem(MEMF_ANY),
               (ULONG)AvailMem(MEMF_LARGEST));
#endif
        function(TextureLoadValue);
#ifdef AMIGACHROME
        Printf((CONST_STRPTR)"AMTEX: done %lu/%lu %s free=%lu largest=%lu\\n",
               AmigaTextureIndex,
               (ULONG)TextureMap.size(),
               tmpAsset.second.TextureFile.c_str(),
               (ULONG)AvailMem(MEMF_ANY),
               (ULONG)AvailMem(MEMF_LARGEST));
#endif
    }
'''
    if old_loop not in texture_text:
        raise RuntimeError("AstroMenace texture preload loop marker missing")
    texture_text = texture_text.replace(old_loop, new_loop, 1)

texture_cpp.write_text(texture_text, encoding="utf-8")

print("patched AstroMenace Amiga random startup and texture/memory diagnostics")
