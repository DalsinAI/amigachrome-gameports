/*
 * AstroMenace AmigaOS 3 launcher.
 *
 * AstroMenace is a large C++/OpenGL application. A normal Amiga Shell or
 * Workbench launch provides only a small stack, and runtime startup or global
 * constructors can exhaust it before main() is reached.
 *
 * Keep the public command as "AstroMenace", but LoadSeg the real program and
 * start it as its own CLI process with CreateNewProcTags(). The child receives
 * a one-megabyte stack before its first instruction, and its CLI default-stack
 * field is set before scheduling it. The launcher waits and returns the real
 * game's return code.
 *
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <string.h>

#define ASTROMENACE_STACK_BYTES (1024UL * 1024UL)
#define ASTROMENACE_ARGUMENT_BYTES 2048

extern struct ExecBase *SysBase;

static struct Task *launcher_task;
static volatile BOOL child_exited;
static volatile LONG child_return_code;
static LONG child_signal_bit = -1;
static ULONG child_signal_mask;
static char child_arguments[ASTROMENACE_ARGUMENT_BYTES];

static BOOL append_character(char **cursor, ULONG *remaining, char value)
{
    if (*remaining <= 1) {
        return FALSE;
    }
    **cursor = value;
    ++*cursor;
    --*remaining;
    return TRUE;
}

static BOOL build_argument_line(int argc, char **argv)
{
    char *cursor = child_arguments;
    ULONG remaining = sizeof(child_arguments);
    int index;

    for (index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        const char *scan;
        BOOL quote = FALSE;

        if (!argument) {
            continue;
        }
        if (index > 1 && !append_character(&cursor, &remaining, ' ')) {
            return FALSE;
        }
        for (scan = argument; *scan; ++scan) {
            if (*scan == ' ' || *scan == '\t' || *scan == '"') {
                quote = TRUE;
                break;
            }
        }
        if (quote && !append_character(&cursor, &remaining, '"')) {
            return FALSE;
        }
        for (scan = argument; *scan; ++scan) {
            /* AmigaDOS uses '*' to quote the following character. */
            if ((*scan == '"' || *scan == '*') &&
                !append_character(&cursor, &remaining, '*')) {
                return FALSE;
            }
            if (!append_character(&cursor, &remaining, *scan)) {
                return FALSE;
            }
        }
        if (quote && !append_character(&cursor, &remaining, '"')) {
            return FALSE;
        }
    }

    if (!append_character(&cursor, &remaining, '\n')) {
        return FALSE;
    }
    *cursor = '\0';
    return TRUE;
}

static BOOL task_is_alive(struct Task *task)
{
    struct Node *node;
    BOOL found = FALSE;

    if (!task) {
        return FALSE;
    }

    Forbid();
    if (SysBase->ThisTask == task) {
        found = TRUE;
    }
    for (node = SysBase->TaskReady.lh_Head;
         !found && node->ln_Succ;
         node = node->ln_Succ) {
        if ((struct Task *)node == task) {
            found = TRUE;
        }
    }
    for (node = SysBase->TaskWait.lh_Head;
         !found && node->ln_Succ;
         node = node->ln_Succ) {
        if ((struct Task *)node == task) {
            found = TRUE;
        }
    }
    Permit();
    return found;
}

/* NP_ExitCode is entered in the dying child with return code in D0 and
 * NP_ExitData in D1 on classic AmigaOS. The callback belongs to the launcher,
 * which remains blocked until the child has completely left Exec's task lists. */
static LONG child_exit_hook(LONG return_code __asm("d0"),
                            LONG exit_data __asm("d1"))
{
    struct Process *process = (struct Process *)FindTask(NULL);

    (void)exit_data;

    /* Mirror shell ownership: cli_Module names the loaded program and is
     * released exactly once as the child exits. */
    if (process && process->pr_CLI) {
        struct CommandLineInterface *cli =
            (struct CommandLineInterface *)BADDR(process->pr_CLI);
        if (cli->cli_Module) {
            UnLoadSeg(cli->cli_Module);
            cli->cli_Module = 0;
        }
    }

    child_return_code = return_code;
    child_exited = TRUE;
    if (launcher_task && child_signal_mask) {
        Signal(launcher_task, child_signal_mask);
    }
    return return_code;
}

static void close_owned_handle(BPTR handle, BOOL owned)
{
    if (owned && handle) {
        Close(handle);
    }
}

int main(int argc, char **argv)
{
    BPTR output = Output();
    BPTR error_output = ErrorOutput();
    BPTR input = Input();
    BPTR program_directory = 0;
    BPTR home_directory = 0;
    BPTR segment = 0;
    BOOL own_input = FALSE;
    BOOL own_output = FALSE;
    struct Process *child_process = NULL;
    struct Task *child_task = NULL;
    ULONG break_signals = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D |
                          SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F;
    LONG create_error = 0;

    if (!build_argument_line(argc, argv)) {
        if (error_output) {
            FPuts(error_output,
                  (CONST_STRPTR)"AstroMenace: command line is too long\n");
        }
        return RETURN_ERROR;
    }

    if (!input) {
        input = Open((CONST_STRPTR)"NIL:", MODE_OLDFILE);
        own_input = TRUE;
    }
    if (!output) {
        output = Open((CONST_STRPTR)"NIL:", MODE_NEWFILE);
        own_output = TRUE;
    }
    if (!error_output) {
        error_output = output;
    }

    program_directory = DupLock(GetProgramDir());
    if (!program_directory) {
        program_directory = Lock((CONST_STRPTR)"PROGDIR:", SHARED_LOCK);
    }
    if (!program_directory || !input || !output) {
        create_error = IoErr();
        if (error_output) {
            FPuts(error_output,
                  (CONST_STRPTR)"AstroMenace: could not prepare launcher resources\n");
            Flush(error_output);
        }
        if (program_directory) {
            UnLock(program_directory);
        }
        close_owned_handle(input, own_input);
        close_owned_handle(output, own_output);
        if (create_error) {
            PrintFault(create_error, (CONST_STRPTR)"AstroMenace");
        }
        return RETURN_FAIL;
    }
    home_directory = DupLock(program_directory);

    segment = LoadSeg((CONST_STRPTR)"PROGDIR:AstroMenace.bin");
    if (!segment) {
        create_error = IoErr();
        if (error_output) {
            FPrintf(error_output,
                    (CONST_STRPTR)"AstroMenace: could not LoadSeg "
                                   "PROGDIR:AstroMenace.bin (IoErr %ld)\n",
                    create_error);
            Flush(error_output);
        }
        if (home_directory) {
            UnLock(home_directory);
        }
        UnLock(program_directory);
        close_owned_handle(input, own_input);
        close_owned_handle(output, own_output);
        return RETURN_FAIL;
    }

    launcher_task = FindTask(NULL);
    child_signal_bit = AllocSignal(-1);
    child_signal_mask = child_signal_bit >= 0 ? (1UL << child_signal_bit) : 0;
    child_exited = FALSE;
    child_return_code = RETURN_FAIL;

    if (output) {
        FPuts(output,
              (CONST_STRPTR)"ASTROLAUNCH: starting AstroMenace.bin with "
                             "1048576-byte stack\n");
        FPuts(output,
              (CONST_STRPTR)"ASTROLAUNCH: CreateNewProcTags AstroMenace.bin\n");
        Flush(output);
    }

    /* The child must not run before cli_DefaultStack and cli_Module have been
     * filled in. Some C startup code uses the CLI value as its stack bound. */
    Forbid();
    child_process = CreateNewProcTags(
        NP_Seglist, (ULONG)segment,
        NP_FreeSeglist, FALSE,
        NP_Name, (ULONG)"AstroMenace",
        NP_CurrentDir, (ULONG)program_directory,
        NP_HomeDir, (ULONG)home_directory,
        NP_Input, (ULONG)input,
        NP_Output, (ULONG)output,
        NP_CloseInput, own_input,
        NP_CloseOutput, own_output,
        NP_Error, (ULONG)error_output,
        NP_CloseError, FALSE,
        NP_Cli, TRUE,
        NP_CommandName, (ULONG)"AstroMenace.bin",
        NP_Arguments, (ULONG)child_arguments,
        NP_StackSize, ASTROMENACE_STACK_BYTES,
        NP_ExitCode, (ULONG)child_exit_hook,
        NP_ExitData, 0,
        TAG_DONE);

    if (child_process && child_process->pr_CLI) {
        struct CommandLineInterface *cli =
            (struct CommandLineInterface *)BADDR(child_process->pr_CLI);
        cli->cli_DefaultStack = ASTROMENACE_STACK_BYTES / sizeof(LONG);
        cli->cli_Module = segment;
        child_task = &child_process->pr_Task;
    }
    Permit();

    if (!child_process || !child_task) {
        create_error = IoErr();
        if (child_process) {
            Signal(&child_process->pr_Task, SIGBREAKF_CTRL_C);
        } else {
            UnLoadSeg(segment);
            if (home_directory) {
                UnLock(home_directory);
            }
            UnLock(program_directory);
            close_owned_handle(input, own_input);
            close_owned_handle(output, own_output);
        }
        if (child_signal_bit >= 0) {
            FreeSignal((BYTE)child_signal_bit);
        }
        if (error_output) {
            FPrintf(error_output,
                    (CONST_STRPTR)"AstroMenace: CreateNewProcTags failed "
                                   "(IoErr %ld)\n",
                    create_error);
            Flush(error_output);
        }
        return RETURN_FAIL;
    }

    while (!child_exited) {
        if (child_signal_mask) {
            ULONG received = Wait(child_signal_mask | break_signals);
            ULONG forwarded = received & break_signals;
            if (forwarded) {
                Forbid();
                if (!child_exited && child_task) {
                    Signal(child_task, forwarded);
                }
                Permit();
            }
        } else {
            Delay(1);
        }
    }

    /* The exit hook lives in this launcher's segment. Do not let the Shell
     * unload us until that callback has returned and the child is gone. */
    while (task_is_alive(child_task)) {
        Delay(1);
    }

    if (child_signal_bit >= 0) {
        FreeSignal((BYTE)child_signal_bit);
    }
    return (int)child_return_code;
}
