#!/usr/bin/env python3
"""Make OpenGPU SDL2's Amiga AHI shutdown safe for linked buffers.

SDL closes an audio device by setting device->shutdown and joining the mixer
thread.  The AHI backend uses two CMD_WRITE requests linked with ahir_Link.
Waiting or aborting those requests one at a time can strand the second request
behind the first, leaving SDL_WaitThread() blocked forever.

During shutdown, abort both linked requests first, then reap both.  ThreadDeinit
uses the same two-phase rule as a final guard.  Ordinary playback is unchanged.
"""

from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()
source = root / "library" / "modules" / "sdl2" / "src" / "audio" / "amigaos3" / "SDL_os3audio.c"
text = source.read_text(encoding="utf-8")

old_wait = '''    if (hidden->inflight) {
        WaitIO((struct IORequest *)hidden->inflight);
    }
    hidden->inflight = hidden->sent;
    hidden->current ^= 1;
'''
new_wait = '''    if (hidden->inflight) {
        struct IORequest *older = (struct IORequest *)hidden->inflight;

        while (!CheckIO(older)) {
            if (SDL_AtomicGet(&_this->shutdown)) {
                struct IORequest *newer = (struct IORequest *)hidden->sent;

                /* ahir_Link makes the newer write depend on the older one.
                   Abort both before waiting for either reply; abort-and-wait
                   one at a time can leave the linked request stranded. */
                if (newer && newer != older && !CheckIO(newer)) {
                    AbortIO(newer);
                }
                if (!CheckIO(older)) {
                    AbortIO(older);
                }
                if (newer && newer != older) {
                    WaitIO(newer);
                }
                WaitIO(older);
                hidden->inflight = NULL;
                hidden->sent = NULL;
                return;
            }
            SDL_Delay(1);
        }
        WaitIO(older);
    }
    hidden->inflight = hidden->sent;
    hidden->current ^= 1;
'''

old_deinit = '''    if (hidden->device_open) {
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
'''
new_deinit = '''    if (hidden->device_open) {
        struct IORequest *active[2] = { NULL, NULL };
        int count = 0;

        /* First abort every outstanding request.  AHI requests are linked;
           waiting for the older request before aborting the newer one can
           deadlock the audio thread during SDL_CloseAudioDevice(). */
        for (i = 0; i < 2; i++) {
            struct IORequest *io = (struct IORequest *)hidden->req[i];
            if (io && (io == (struct IORequest *)hidden->inflight ||
                       io == (struct IORequest *)hidden->sent)) {
                active[count++] = io;
                if (!CheckIO(io)) {
                    AbortIO(io);
                }
            }
        }
        /* Then reap all replies only after every linked request has received
           its abort. */
        for (i = count - 1; i >= 0; i--) {
            WaitIO(active[i]);
        }
        hidden->inflight = hidden->sent = NULL;
        CloseDevice((struct IORequest *)hidden->req[0]);
        hidden->device_open = 0;
    }
'''

if new_wait not in text:
    if old_wait not in text:
        raise RuntimeError("OpenGPU SDL2 AHI WaitDevice anchor not found")
    text = text.replace(old_wait, new_wait, 1)

if new_deinit not in text:
    if old_deinit not in text:
        raise RuntimeError("OpenGPU SDL2 AHI ThreadDeinit anchor not found")
    text = text.replace(old_deinit, new_deinit, 1)

source.write_text(text, encoding="utf-8")
print(f"patched linked-request AHI shutdown: {source}")
