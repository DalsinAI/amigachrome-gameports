/* OpenFormula 68K - procedural AHI engine audio.
 * SPDX-License-Identifier: MIT
 */
#include "audio_ahi.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stdint.h>
#include <string.h>

#define OF_AUDIO_RATE 44100U
#define OF_AUDIO_CHUNK 882U

static struct Task *of_main_task;
static BYTE of_sound_sig = -1;
static volatile int of_sound_quit;
static volatile int of_sound_state;
static volatile unsigned of_rpm;
static volatile unsigned of_throttle;
static volatile unsigned of_speed;
static volatile int of_steer;
static volatile unsigned of_beep_kind;
static volatile unsigned of_hit_level;

static int clamp16(int value)
{
    if (value < -32768) return -32768;
    if (value > 32767) return 32767;
    return value;
}

static void mix_chunk(int16_t *dst, unsigned frames)
{
    static uint32_t engine_phase;
    static uint32_t harmonic_phase;
    static uint32_t wind_phase;
    static uint32_t noise = 0x13579bdfUL;
    static unsigned beep_left;
    static unsigned beep_phase;
    static unsigned hit_left;
    unsigned rpm, throttle, speed, beep, hit;
    int steer;
    unsigned i;

    Forbid();
    rpm = of_rpm;
    throttle = of_throttle;
    speed = of_speed;
    steer = of_steer;
    beep = of_beep_kind;
    hit = of_hit_level;
    of_beep_kind = 0;
    of_hit_level = 0;
    Permit();

    if (beep) {
        beep_left = OF_AUDIO_RATE / (beep == 4 ? 3U : 7U);
        beep_phase = 0;
    }
    if (hit)
        hit_left = OF_AUDIO_RATE / 9U;

    for (i = 0; i < frames; ++i) {
        unsigned engine_hz = 45U + rpm / 18U;
        unsigned harmonic_hz = engine_hz * 2U + 17U;
        int saw, square, wind, tone = 0, crash = 0;
        int mono, pan, left, right;

        engine_phase += (uint32_t)(((uint64_t)engine_hz << 32) / OF_AUDIO_RATE);
        harmonic_phase += (uint32_t)(((uint64_t)harmonic_hz << 32) / OF_AUDIO_RATE);
        wind_phase += (uint32_t)(((uint64_t)(120U + speed / 90U) << 32) / OF_AUDIO_RATE);

        saw = (int)((engine_phase >> 16) & 0xffffU) - 32768;
        square = (harmonic_phase & 0x80000000UL) ? 11000 : -11000;
        noise ^= noise << 13;
        noise ^= noise >> 17;
        noise ^= noise << 5;
        wind = ((int)(noise & 0xffffU) - 32768) * (int)(speed > 12000U ? 12000U : speed) / 12000;

        if (beep_left) {
            unsigned hz = beep == 4 ? 1250U : 880U;
            beep_phase += (unsigned)(((uint64_t)hz << 24) / OF_AUDIO_RATE);
            tone = (beep_phase & 0x800000U) ? 13000 : -13000;
            --beep_left;
        }
        if (hit_left) {
            crash = ((int)(noise & 0xffffU) - 32768) * (int)hit_left / (int)(OF_AUDIO_RATE / 9U);
            --hit_left;
        }

        mono = (saw * (2800 + (int)throttle * 28) >> 15)
             + (square * (1500 + (int)throttle * 12) >> 15)
             + (wind >> 4) + tone + (crash >> 1);
        pan = steer;
        if (pan < -32768) pan = -32768;
        if (pan > 32767) pan = 32767;
        left = mono * (49152 - pan / 2) / 49152;
        right = mono * (49152 + pan / 2) / 49152;
        dst[i * 2 + 0] = (int16_t)clamp16(left);
        dst[i * 2 + 1] = (int16_t)clamp16(right);
    }
}

static void sound_main(void)
{
    struct MsgPort *port = NULL;
    struct AHIRequest *io[2] = { NULL, NULL };
    struct AHIRequest *previous = NULL;
    int16_t *buffer[2] = { NULL, NULL };
    int index = 0;
    int opened = 0;

    port = CreateMsgPort();
    if (!port)
        goto done;
    io[0] = (struct AHIRequest *)CreateIORequest(port, sizeof(struct AHIRequest));
    if (!io[0])
        goto done;
    io[0]->ahir_Version = 4;
    if (OpenDevice((CONST_STRPTR)AHINAME, 0, (struct IORequest *)io[0], 0))
        goto done;
    opened = 1;
    io[1] = (struct AHIRequest *)AllocVec(sizeof(struct AHIRequest), MEMF_PUBLIC | MEMF_CLEAR);
    buffer[0] = (int16_t *)AllocVec(OF_AUDIO_CHUNK * 4U, MEMF_PUBLIC | MEMF_CLEAR);
    buffer[1] = (int16_t *)AllocVec(OF_AUDIO_CHUNK * 4U, MEMF_PUBLIC | MEMF_CLEAR);
    if (!io[1] || !buffer[0] || !buffer[1])
        goto done;
    CopyMem(io[0], io[1], sizeof(struct AHIRequest));

    of_sound_state = 1;
    Signal(of_main_task, 1UL << of_sound_sig);

    while (!of_sound_quit) {
        mix_chunk(buffer[index], OF_AUDIO_CHUNK);
        io[index]->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
        io[index]->ahir_Std.io_Command = CMD_WRITE;
        io[index]->ahir_Std.io_Data = buffer[index];
        io[index]->ahir_Std.io_Length = OF_AUDIO_CHUNK * 4U;
        io[index]->ahir_Std.io_Offset = 0;
        io[index]->ahir_Type = AHIST_S16S;
        io[index]->ahir_Frequency = OF_AUDIO_RATE;
        io[index]->ahir_Volume = 0x10000UL;
        io[index]->ahir_Position = 0x8000UL;
        io[index]->ahir_Link = previous;
        SendIO((struct IORequest *)io[index]);
        if (previous)
            WaitIO((struct IORequest *)previous);
        previous = io[index];
        index ^= 1;
    }

    if (previous) {
        AbortIO((struct IORequest *)previous);
        WaitIO((struct IORequest *)previous);
    }

done:
    if (opened)
        CloseDevice((struct IORequest *)io[0]);
    FreeVec(buffer[0]);
    FreeVec(buffer[1]);
    FreeVec(io[1]);
    if (io[0])
        DeleteIORequest((struct IORequest *)io[0]);
    if (port)
        DeleteMsgPort(port);

    Forbid();
    of_sound_state = of_sound_state == 1 ? 2 : -1;
    if (of_main_task && of_sound_sig >= 0)
        Signal(of_main_task, 1UL << of_sound_sig);
}

int of_audio_start(void)
{
    of_main_task = FindTask(NULL);
    of_sound_quit = 0;
    of_sound_state = 0;
    of_rpm = 1000;
    of_throttle = 0;
    of_speed = 0;
    of_steer = 0;
    of_beep_kind = 0;
    of_hit_level = 0;

    of_sound_sig = AllocSignal(-1);
    if (of_sound_sig < 0)
        return 0;
    SetSignal(0, 1UL << of_sound_sig);
    if (!CreateNewProcTags(NP_Entry, (ULONG)sound_main,
                           NP_Name, (ULONG)"OpenFormula AHI",
                           NP_Priority, 5,
                           NP_StackSize, 32768,
                           TAG_END)) {
        FreeSignal(of_sound_sig);
        of_sound_sig = -1;
        return 0;
    }
    while (!of_sound_state)
        Wait(1UL << of_sound_sig);
    return of_sound_state == 1;
}

void of_audio_stop(void)
{
    if (of_sound_state == 1) {
        of_sound_quit = 1;
        while (of_sound_state == 1)
            Wait(1UL << of_sound_sig);
    }
    if (of_sound_sig >= 0) {
        FreeSignal(of_sound_sig);
        of_sound_sig = -1;
    }
}

int of_audio_available(void)
{
    return of_sound_state == 1;
}

void of_audio_set_engine(unsigned rpm, unsigned throttle, unsigned speed, int steer)
{
    of_rpm = rpm;
    of_throttle = throttle;
    of_speed = speed;
    of_steer = steer;
}

void of_audio_beep(unsigned kind)
{
    of_beep_kind = kind ? kind : 1;
}

void of_audio_hit(void)
{
    of_hit_level = 32767U;
}
