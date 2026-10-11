/*
 * AstroMenace AmigaOS 3 launcher.
 *
 * AstroMenace is a large C++/OpenGL application. The qualified AC090 path
 * needs substantially more stack than a normal Amiga Shell or Workbench
 * launch provides. Keep the public command as "AstroMenace", load the real
 * program, then invoke it synchronously with DOS RunCommand() and the tested
 * one-megabyte stack. This puts the program entry, C++ constructors and main()
 * on the replacement stack from their first instruction.
 *
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#define ASTROMENACE_STACK_BYTES (1024UL * 1024UL)

int main(void)
{
    static const char arguments[] = "\n";
    BPTR output = Output();
    BPTR error_output = ErrorOutput();
    BPTR segment;
    LONG result;

    if (output) {
        FPrintf(output,
                (CONST_STRPTR)"ASTROLAUNCH: RunCommand AstroMenace.bin with "
                                 "%ld-byte stack\n",
                ASTROMENACE_STACK_BYTES);
        Flush(output);
    }

    segment = LoadSeg((CONST_STRPTR)"PROGDIR:AstroMenace.bin");
    if (!segment) {
        BPTR destination = error_output ? error_output : output;
        if (destination) {
            FPrintf(destination,
                    (CONST_STRPTR)"AstroMenace: could not LoadSeg "
                                     "PROGDIR:AstroMenace.bin (IoErr %ld)\n",
                    IoErr());
            Flush(destination);
        }
        return RETURN_FAIL;
    }

    result = RunCommand(segment,
                        ASTROMENACE_STACK_BYTES,
                        (CONST_STRPTR)arguments,
                        (ULONG)(sizeof(arguments) - 1));

    if (result == -1) {
        BPTR destination = error_output ? error_output : output;
        if (destination) {
            FPrintf(destination,
                    (CONST_STRPTR)"AstroMenace: RunCommand failed "
                                     "(IoErr %ld)\n",
                    IoErr());
            Flush(destination);
        }
        result = RETURN_FAIL;
    }

    UnLoadSeg(segment);
    return (int)result;
}
