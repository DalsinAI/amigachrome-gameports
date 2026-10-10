#!/usr/bin/env python3
"""Remove AstroMenace's pre-main std::random_device dependency on AmigaOS.

AstroMenace deliberately uses the random engine from global constructors.  The
upstream GCC 16 fix correctly moved the engine to a function-local static, but
it still seeds that engine with std::random_device on first use.  The classic
AmigaOS libstdc++ build has no reliable random_device entropy backend; its
constructor can therefore throw while global constructors are running.  That
happens before main(), so Workbench can show only "Program aborted" and none
of the normal AMDBG markers are reached.

For the AmigaChrome target we initialise with a safe non-zero seed and reseed
once SDL's timer subsystem is alive in main().  Other platforms keep the
upstream std::random_device behaviour unchanged.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()

rand_cpp = source / "src" / "core" / "math" / "rand.cpp"
math_h = source / "src" / "core" / "math" / "math.h"
main_cpp = source / "src" / "main.cpp"

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
main_cpp.write_text(main_text, encoding="utf-8")

print("patched AstroMenace Amiga random startup: no std::random_device before main")
