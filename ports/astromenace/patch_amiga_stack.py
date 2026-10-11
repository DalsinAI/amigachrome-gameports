#!/usr/bin/env python3
"""Make AstroMenace independent of the launching Shell's tiny stack.

AmigaOS 3.x commonly launches commands with a 4 KiB stack.  The runtime
qualification had been hiding that by issuing ``Stack 1000000`` before the
game.  A release user launching ``AstroMenace`` directly therefore exercised
a materially different path and could abort before a visible window.

This patch turns the upstream main into a body function and installs the same
well-tested Exec StackSwap helper used by the AmigaChrome guest tools.  The
body receives a private 1 MiB stack when the caller supplied less.  It still
returns normally to the original stack; it must not call exit().
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
main_cpp = source / "src" / "main.cpp"
platform_dir = source / "src" / "core" / "platform"
header = platform_dir / "amigachrome_stack.h"
implementation = platform_dir / "amigachrome_stack.cpp"

platform_dir.mkdir(parents=True, exist_ok=True)

header.write_text(
    r'''/* SPDX-License-Identifier: MIT */
#ifndef ASTROMENACE_AMIGACHROME_STACK_H
#define ASTROMENACE_AMIGACHROME_STACK_H

int AMRunMainWithStack(int (*body)(int, char **),
                       int argc,
                       char **argv,
                       unsigned long bytes);

#endif
''',
    encoding="utf-8",
)

implementation.write_text(
    r'''/* SPDX-License-Identifier: MIT */
/* AstroMenace AmigaChrome private-stack startup helper. */
#include "amigachrome_stack.h"

#ifdef AMIGACHROME
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* Static by design: nothing between the two StackSwap calls may depend on
 * the old stack frame.  AstroMenace enters this helper once at startup. */
static struct StackSwapStruct AMStackSwap;
static int (*AMStackBody)(int, char **);
static int AMStackArgc;
static int AMStackResult;
static char **AMStackArgv;

/* Keep the body's argument push/pop wholly on the replacement stack. */
static void __attribute__((noinline)) AMCallBody(void)
{
    AMStackResult = AMStackBody(AMStackArgc, AMStackArgv);
}

static void __attribute__((noinline)) AMRunSwapped(void)
{
    StackSwap(&AMStackSwap);
    AMCallBody();
    StackSwap(&AMStackSwap);
}
#endif

int AMRunMainWithStack(int (*body)(int, char **),
                       int argc,
                       char **argv,
                       unsigned long bytes)
{
#ifdef AMIGACHROME
    struct Task *task = FindTask(NULL);
    APTR lower;

    if ((ULONG)task->tc_SPUpper - (ULONG)task->tc_SPLower >= bytes) {
        return body(argc, argv);
    }

    lower = AllocVec(bytes, MEMF_ANY);
    if (!lower) {
        /* Preserve the normal error path if memory is critically low. */
        return body(argc, argv);
    }

    AMStackSwap.stk_Lower = lower;
    AMStackSwap.stk_Upper = (ULONG)lower + bytes;
    AMStackSwap.stk_Pointer = (APTR)AMStackSwap.stk_Upper;
    AMStackBody = body;
    AMStackArgc = argc;
    AMStackArgv = argv;
    AMRunSwapped();
    FreeVec(lower);
    return AMStackResult;
#else
    (void)bytes;
    return body(argc, argv);
#endif
}
''',
    encoding="utf-8",
)

text = main_cpp.read_text(encoding="utf-8")
include = '#include "core/platform/amigachrome_stack.h"\n'
if include not in text:
    anchor = '#include "SDL2/SDL.h"\n'
    if anchor not in text:
        raise RuntimeError("AstroMenace SDL include marker missing for stack helper")
    text = text.replace(anchor, anchor + include, 1)

body_signature = "static int AstroMenaceBody(int argc, char *argv[])\n{"
if body_signature not in text:
    main_signature = "int main(int argc, char *argv[])\n{"
    if main_signature not in text:
        raise RuntimeError("AstroMenace main marker missing for stack helper")
    text = text.replace(main_signature, body_signature, 1)

wrapper_marker = "AMRunMainWithStack(AstroMenaceBody"
if wrapper_marker not in text:
    text = text.rstrip() + r'''

/*
 * A Shell, User-Startup or Workbench launch must all be equivalent.  The
 * upstream game body is run with a private stack instead of relying on the
 * caller to remember a Stack command or an icon Stack field.
 */
int main(int argc, char *argv[])
{
    return AMRunMainWithStack(AstroMenaceBody,
                              argc,
                              argv,
                              1024UL * 1024UL);
}
''' + "\n"

main_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace startup to provide a private 1 MiB Amiga stack")
