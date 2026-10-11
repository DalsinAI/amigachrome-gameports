#!/usr/bin/env python3
"""Instrument the prepared OpenGPU SDL2/SDL2_mixer trees at shutdown.

This is a diagnostic-only source transformation.  It writes small PROGDIR:
stage files around every SDL_mixer close operation, SDL_CloseAudioDevice's
lock/thread/device phases, and the Amiga AHI wait/deinitialisation path.  The
result pinpoints the first blocking call without relying on buffered stdout.
"""

from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch_mixclose_probe_modules.py OUT_DIR")

out = Path(sys.argv[1]).resolve()
work = out / "sdl2-work"
mixer = work / "SDL2_mixer-2.8.2" / "src" / "mixer.c"
sdl_audio = work / "SDL2-2.32.10" / "src" / "audio" / "SDL_audio.c"
ahi = work / "SDL2-2.32.10" / "src" / "audio" / "amigaos3" / "SDL_os3audio.c"

for source in (mixer, sdl_audio, ahi):
    if not source.is_file():
        raise RuntimeError(f"prepared source missing: {source}")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)


# ---- SDL2_mixer -------------------------------------------------------------
text = mixer.read_text(encoding="utf-8")
helper_anchor = '#include "effects_internal.h"\n'
helper = r'''#include "effects_internal.h"

#if defined(__AMIGAOS3__)
static void AMMixCloseStage(const char *path)
{
    SDL_RWops *rw = SDL_RWFromFile(path, "wb");
    if (rw) {
        const char ready[] = "ready\n";
        SDL_RWwrite(rw, ready, 1, sizeof(ready) - 1);
        SDL_RWclose(rw);
    }
}

static void AMMixCloseChannelStage(const char *phase, int channel)
{
    char path[96];
    SDL_snprintf(path, sizeof(path),
                 "PROGDIR:stage-mixclose-effects-%02d-%s",
                 channel, phase);
    AMMixCloseStage(path);
}
#else
#define AMMixCloseStage(path) ((void)0)
#define AMMixCloseChannelStage(phase, channel) ((void)0)
#endif
'''
text = replace_once(text, helper_anchor, helper, "mixer helper")

old_close = r'''/* Close the mixer, halting all playing audio */
void Mix_CloseAudio(void)
{
    int i;

    if (audio_opened) {
        if (audio_opened == 1) {
            for (i = 0; i < num_channels; i++) {
                Mix_UnregisterAllEffects(i);
            }
            Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
            close_music();
            Mix_SetMusicCMD(NULL);
            Mix_HaltChannel(-1);
            _Mix_DeinitEffects();
            SDL_CloseAudioDevice(audio_device);
            audio_device = 0;
            SDL_free(mix_channel);
            mix_channel = NULL;

            /* rcg06042009 report available decoders at runtime. */
            SDL_free((void *)chunk_decoders);
            chunk_decoders = NULL;
            num_decoders = 0;
        }
        --audio_opened;
    }
}
'''
new_close = r'''/* Close the mixer, halting all playing audio */
void Mix_CloseAudio(void)
{
    int i;

    AMMixCloseStage("PROGDIR:stage-mixclose-00-enter");
    if (audio_opened) {
        AMMixCloseStage("PROGDIR:stage-mixclose-01-audio-opened");
        if (audio_opened == 1) {
            AMMixCloseStage("PROGDIR:stage-mixclose-02-before-channel-effects");
            for (i = 0; i < num_channels; i++) {
                AMMixCloseChannelStage("before", i);
                Mix_UnregisterAllEffects(i);
                AMMixCloseChannelStage("after", i);
            }
            AMMixCloseStage("PROGDIR:stage-mixclose-03-after-channel-effects");
            AMMixCloseStage("PROGDIR:stage-mixclose-04-before-post-effects");
            Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
            AMMixCloseStage("PROGDIR:stage-mixclose-05-after-post-effects");
            AMMixCloseStage("PROGDIR:stage-mixclose-06-before-close-music");
            close_music();
            AMMixCloseStage("PROGDIR:stage-mixclose-07-after-close-music");
            AMMixCloseStage("PROGDIR:stage-mixclose-08-before-music-cmd");
            Mix_SetMusicCMD(NULL);
            AMMixCloseStage("PROGDIR:stage-mixclose-09-after-music-cmd");
            AMMixCloseStage("PROGDIR:stage-mixclose-10-before-halt-channels");
            Mix_HaltChannel(-1);
            AMMixCloseStage("PROGDIR:stage-mixclose-11-after-halt-channels");
            AMMixCloseStage("PROGDIR:stage-mixclose-12-before-deinit-effects");
            _Mix_DeinitEffects();
            AMMixCloseStage("PROGDIR:stage-mixclose-13-after-deinit-effects");
            AMMixCloseStage("PROGDIR:stage-mixclose-14-before-sdl-close-device");
            SDL_CloseAudioDevice(audio_device);
            AMMixCloseStage("PROGDIR:stage-mixclose-15-after-sdl-close-device");
            audio_device = 0;
            SDL_free(mix_channel);
            mix_channel = NULL;
            AMMixCloseStage("PROGDIR:stage-mixclose-16-after-channel-free");

            /* rcg06042009 report available decoders at runtime. */
            SDL_free((void *)chunk_decoders);
            chunk_decoders = NULL;
            num_decoders = 0;
            AMMixCloseStage("PROGDIR:stage-mixclose-17-after-decoder-free");
        }
        --audio_opened;
    }
    AMMixCloseStage("PROGDIR:stage-mixclose-18-return");
}
'''
text = replace_once(text, old_close, new_close, "Mix_CloseAudio")
mixer.write_text(text, encoding="utf-8")

# ---- SDL core audio close ---------------------------------------------------
text = sdl_audio.read_text(encoding="utf-8")
core_anchor = '#define _THIS SDL_AudioDevice *_this\n'
core_helper = r'''#define _THIS SDL_AudioDevice *_this

#if defined(__AMIGAOS3__)
static void ACSDLAudioCloseStage(const char *path)
{
    SDL_RWops *rw = SDL_RWFromFile(path, "wb");
    if (rw) {
        const char ready[] = "ready\n";
        SDL_RWwrite(rw, ready, 1, sizeof(ready) - 1);
        SDL_RWclose(rw);
    }
}
#else
#define ACSDLAudioCloseStage(path) ((void)0)
#endif
'''
text = replace_once(text, core_anchor, core_helper, "SDL audio helper")

old_core_close = r'''static void close_audio_device(SDL_AudioDevice *device)
{
    if (!device) {
        return;
    }

    /* make sure the device is paused before we do anything else, so the
       audio callback definitely won't fire again. */
    current_audio.impl.LockDevice(device);
    SDL_AtomicSet(&device->paused, 1);
    SDL_AtomicSet(&device->shutdown, 1);
    SDL_AtomicSet(&device->enabled, 0);
    current_audio.impl.UnlockDevice(device);

    if (device->thread != NULL) {
        SDL_WaitThread(device->thread, NULL);
    }
    if (device->mixer_lock != NULL) {
        SDL_DestroyMutex(device->mixer_lock);
    }

    SDL_free(device->work_buffer);
    SDL_FreeAudioStream(device->stream);

    if (device->id > 0) {
        SDL_AudioDevice *opendev = open_devices[device->id - 1];
        SDL_assert((opendev == device) || (opendev == NULL));
        if (opendev == device) {
            open_devices[device->id - 1] = NULL;
        }
    }

    if (device->hidden != NULL) {
        current_audio.impl.CloseDevice(device);
    }

    SDL_FreeDataQueue(device->buffer_queue);

    SDL_free(device);
}
'''
new_core_close = r'''static void close_audio_device(SDL_AudioDevice *device)
{
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-00-enter");
    if (!device) {
        ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-01-null-device");
        return;
    }

    /* make sure the device is paused before we do anything else, so the
       audio callback definitely won't fire again. */
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-02-before-lock");
    current_audio.impl.LockDevice(device);
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-03-after-lock");
    SDL_AtomicSet(&device->paused, 1);
    SDL_AtomicSet(&device->shutdown, 1);
    SDL_AtomicSet(&device->enabled, 0);
    current_audio.impl.UnlockDevice(device);
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-04-after-unlock");

    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-05-before-wait-thread");
    if (device->thread != NULL) {
        SDL_WaitThread(device->thread, NULL);
    }
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-06-after-wait-thread");
    if (device->mixer_lock != NULL) {
        SDL_DestroyMutex(device->mixer_lock);
    }
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-07-after-mutex-destroy");

    SDL_free(device->work_buffer);
    SDL_FreeAudioStream(device->stream);
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-08-after-buffer-free");

    if (device->id > 0) {
        SDL_AudioDevice *opendev = open_devices[device->id - 1];
        SDL_assert((opendev == device) || (opendev == NULL));
        if (opendev == device) {
            open_devices[device->id - 1] = NULL;
        }
    }
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-09-after-device-table");

    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-10-before-driver-close");
    if (device->hidden != NULL) {
        current_audio.impl.CloseDevice(device);
    }
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-11-after-driver-close");

    SDL_FreeDataQueue(device->buffer_queue);
    SDL_free(device);
    ACSDLAudioCloseStage("PROGDIR:stage-sdlaudio-12-return");
}
'''
text = replace_once(text, old_core_close, new_core_close, "SDL close_audio_device")
sdl_audio.write_text(text, encoding="utf-8")

# ---- AHI thread/wait path ---------------------------------------------------
text = ahi.read_text(encoding="utf-8")
ahi_anchor = '#define OS3AHI_DRIVER_NAME "ahi"\n'
ahi_helper = r'''#define OS3AHI_DRIVER_NAME "ahi"

static void OS3AHI_CloseStage(const char *path)
{
    SDL_RWops *rw = SDL_RWFromFile(path, "wb");
    if (rw) {
        const char ready[] = "ready\n";
        SDL_RWwrite(rw, ready, 1, sizeof(ready) - 1);
        SDL_RWclose(rw);
    }
}
'''
text = replace_once(text, ahi_anchor, ahi_helper, "AHI helper")

old_wait = r'''    if (hidden->inflight) {
        struct IORequest *io = (struct IORequest *)hidden->inflight;

        /* SDL_CloseAudioDevice() sets shutdown before it waits for this
           thread.  A blocking WaitIO() cannot observe that request and can
           strand the closer forever if an AHI linked write does not finish.
           Poll cheaply during ordinary playback; on shutdown, abort and reap
           the request in the audio thread that owns the reply port. */
        while (!CheckIO(io)) {
            if (SDL_AtomicGet(&_this->shutdown)) {
                AbortIO(io);
                break;
            }
            SDL_Delay(1);
        }
        WaitIO(io);
    }
'''
new_wait = r'''    if (hidden->inflight) {
        struct IORequest *io = (struct IORequest *)hidden->inflight;

        /* SDL_CloseAudioDevice() sets shutdown before it waits for this
           thread.  A blocking WaitIO() cannot observe that request and can
           strand the closer forever if an AHI linked write does not finish.
           Poll cheaply during ordinary playback; on shutdown, abort and reap
           the request in the audio thread that owns the reply port. */
        while (!CheckIO(io)) {
            if (SDL_AtomicGet(&_this->shutdown)) {
                OS3AHI_CloseStage("PROGDIR:stage-ahi-wait-00-shutdown-seen");
                AbortIO(io);
                OS3AHI_CloseStage("PROGDIR:stage-ahi-wait-01-abort-issued");
                break;
            }
            SDL_Delay(1);
        }
        OS3AHI_CloseStage("PROGDIR:stage-ahi-wait-02-before-waitio");
        WaitIO(io);
        OS3AHI_CloseStage("PROGDIR:stage-ahi-wait-03-after-waitio");
    }
'''
text = replace_once(text, old_wait, new_wait, "AHI WaitDevice")

old_deinit = r'''static void OS3AHI_ThreadDeinit(_THIS)
{
    struct SDL_PrivateAudioData *hidden = _this->hidden;
    int i;

    if (hidden->device_open) {
        for (i = 0; i < 2; i++) {
            struct IORequest *io = (struct IORequest *)hidden->req[i];
            if (io && (io == (struct IORequest *)hidden->inflight || io == (struct IORequest *)hidden->sent)) {
                if (!CheckIO(io)) {
                    AbortIO(io);
                }
                WaitIO(io);
            }
        }
        hidden->inflight = hidden->sent = NULL;
        CloseDevice((struct IORequest *)hidden->req[0]);
        hidden->device_open = 0;
    }
    if (hidden->req[1]) {
        FreeVec(hidden->req[1]);
        hidden->req[1] = NULL;
    }
    if (hidden->req[0]) {
        DeleteIORequest((struct IORequest *)hidden->req[0]);
        hidden->req[0] = NULL;
    }
    if (hidden->port) {
        DeleteMsgPort(hidden->port);
        hidden->port = NULL;
    }
}
'''
new_deinit = r'''static void OS3AHI_ThreadDeinit(_THIS)
{
    struct SDL_PrivateAudioData *hidden = _this->hidden;
    int i;

    OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-00-enter");
    if (hidden->device_open) {
        for (i = 0; i < 2; i++) {
            struct IORequest *io = (struct IORequest *)hidden->req[i];
            if (io && (io == (struct IORequest *)hidden->inflight || io == (struct IORequest *)hidden->sent)) {
                if (i == 0) OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-01-request0");
                else OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-04-request1");
                if (!CheckIO(io)) {
                    AbortIO(io);
                    if (i == 0) OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-02-abort0");
                    else OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-05-abort1");
                }
                WaitIO(io);
                if (i == 0) OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-03-wait0-done");
                else OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-06-wait1-done");
            }
        }
        hidden->inflight = hidden->sent = NULL;
        OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-07-before-close-device");
        CloseDevice((struct IORequest *)hidden->req[0]);
        OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-08-after-close-device");
        hidden->device_open = 0;
    }
    if (hidden->req[1]) {
        FreeVec(hidden->req[1]);
        hidden->req[1] = NULL;
    }
    if (hidden->req[0]) {
        DeleteIORequest((struct IORequest *)hidden->req[0]);
        hidden->req[0] = NULL;
    }
    if (hidden->port) {
        DeleteMsgPort(hidden->port);
        hidden->port = NULL;
    }
    OS3AHI_CloseStage("PROGDIR:stage-ahi-deinit-09-return");
}
'''
text = replace_once(text, old_deinit, new_deinit, "AHI ThreadDeinit")
ahi.write_text(text, encoding="utf-8")

print(f"instrumented {mixer}")
print(f"instrumented {sdl_audio}")
print(f"instrumented {ahi}")
