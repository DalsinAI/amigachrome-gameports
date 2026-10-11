/*
 * AstroMenace AmigaOS 3 launcher.
 *
 * The game is a large C++/OpenGL application and its qualified AC090 launch
 * path needs substantially more stack than a normal Amiga Shell supplies.
 * Keep the user-facing command as "AstroMenace" while starting the real
 * program, AstroMenace.bin, with the same 1 MiB stack used by qualification.
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
    LONG result;

    result = SystemTags((CONST_STRPTR)"PROGDIR:AstroMenace.bin",
                        NP_StackSize, ASTROMENACE_STACK_BYTES,
                        SYS_Asynch, FALSE,
                        TAG_DONE);

    if (result == -1) {
        BPTR error_output = ErrorOutput();
        if (error_output) {
            FPrintf(error_output,
                    (CONST_STRPTR)"AstroMenace: could not start "
                                     "PROGDIR:AstroMenace.bin (IoErr %ld)\n",
                    IoErr());
        }
        return RETURN_FAIL;
    }

    return (int)result;
}
