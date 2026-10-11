#!/usr/bin/env python3
"""Patch OpenGPU SDL2's Amiga AHI backend for interruptible shutdown.

SDL 2 closes an audio device by setting device->shutdown and then joining the
mixer thread.  The Amiga AHI backend previously blocked that thread inside a
single WaitIO() call.  If the linked double-buffer chain did not complete,
SDL_WaitThread() could wait forever and Mix_CloseAudio() never returned.

Poll CheckIO() at a low rate instead.  Once SDL requests shutdown, abort the
outstanding request from the audio thread itself and reap it with WaitIO().
Normal playback is unchanged: the code still waits until the buffer completes.
"""

from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()
source = root / "library" / "modules" / "sdl2" / "src" / "audio" / "amigaos3" / "SDL_os3audio.c"
text = source.read_text(encoding="utf-8")

old = '''    if (hidden->inflight) {
        WaitIO((struct IORequest *)hidden->inflight);
    }
    hidden->inflight = hidden->sent;
    hidden->current ^= 1;
'''

new = '''    if (hidden->inflight) {
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
    hidden->inflight = hidden->sent;
    hidden->current ^= 1;
'''

if new in text:
    print("OpenGPU SDL2 AHI close patch already present")
    raise SystemExit(0)
if old not in text:
    raise RuntimeError("OpenGPU SDL2 AHI WaitDevice anchor not found")

source.write_text(text.replace(old, new, 1), encoding="utf-8")
print(f"patched interruptible SDL2 AHI close: {source}")
