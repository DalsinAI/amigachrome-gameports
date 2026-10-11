/*
 * AstroMenace AmigaOS launcher.
 *
 * The game is a large C++ program and can execute runtime initialisation
 * before main().  A normal Amiga Shell supplies only a small stack, so a
 * StackSwap performed by the game's main function is already too late.
 *
 * This tiny C program is the user-facing AstroMenace command.  It starts the
 * real AstroMenace.bin synchronously with a 1 MiB process stack, before the
 * child's C/C++ runtime or global constructors execute.
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
    static const char command[] = "AstroMenace.bin";
    BPTR current_dir = DupLock(GetProgramDir());
    BPTR input = Input();
    BPTR output = Output();
    LONG result;

    if (!current_dir) {
        current_dir = Lock((CONST_STRPTR)"PROGDIR:", SHARED_LOCK);
    }

    if (!current_dir) {
        if (output) {
            FPuts(output,
                  (CONST_STRPTR)"AstroMenace: cannot lock PROGDIR:\n");
        }
        PrintFault(IoErr(), (CONST_STRPTR)"AstroMenace");
        return RETURN_FAIL;
    }

    if (output) {
        FPuts(output,
              (CONST_STRPTR)"AMLAUNCH: AstroMenace.bin with 1048576-byte child stack\n");
        Flush(output);
    }

    result = SystemTags((CONST_STRPTR)command,
                        SYS_Input, (ULONG)input,
                        SYS_Output, (ULONG)output,
                        NP_Error, (ULONG)output,
                        SYS_Asynch, FALSE,
                        SYS_UserShell, FALSE,
                        NP_CurrentDir, (ULONG)current_dir,
                        NP_CommandName, (ULONG)"AstroMenace",
                        NP_StackSize, ASTROMENACE_STACK_BYTES,
                        TAG_DONE);

    /* SystemTags owns the duplicate current-directory lock after a successful
     * launch.  On failure the child was not created, so release it here. */
    if (result == -1) {
        LONG error = IoErr();
        UnLock(current_dir);
        if (output) {
            FPuts(output,
                  (CONST_STRPTR)"AstroMenace: launcher could not start AstroMenace.bin\n");
            Flush(output);
        }
        PrintFault(error, (CONST_STRPTR)"AstroMenace");
        return RETURN_FAIL;
    }

    return (int)result;
}
