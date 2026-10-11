/*
 * AstroMenace AmigaOS 3 launcher.
 *
 * AstroMenace is a large C++/OpenGL application.  The qualified AC090 path
 * needs substantially more stack than a normal Amiga Shell or Workbench
 * launch provides.  Keep the public command as "AstroMenace", but create the
 * real program as a fresh child process with the tested one-megabyte stack.
 * This avoids attempting to StackSwap a running C++ process.
 *
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#define ASTROMENACE_STACK_BYTES (1024UL * 1024UL)

int main(void)
{
    BPTR input = Input();
    BPTR output = Output();
    BPTR error_output = ErrorOutput();
    LONG result;

    if (output) {
        FPrintf(output,
                (CONST_STRPTR)"ASTROLAUNCH: starting AstroMenace.bin with "
                                 "%ld-byte stack\n",
                ASTROMENACE_STACK_BYTES);
    }

    result = SystemTags((CONST_STRPTR)"PROGDIR:AstroMenace.bin",
                        NP_StackSize, ASTROMENACE_STACK_BYTES,
                        SYS_Input, input,
                        SYS_Output, output,
                        SYS_Error, error_output,
                        SYS_Asynch, FALSE,
                        TAG_DONE);

    if (result == -1) {
        BPTR destination = error_output ? error_output : output;
        if (destination) {
            FPrintf(destination,
                    (CONST_STRPTR)"AstroMenace: could not start "
                                     "PROGDIR:AstroMenace.bin (IoErr %ld)\n",
                    IoErr());
        }
        return RETURN_FAIL;
    }

    return (int)result;
}
