#!/usr/bin/env python3
"""Add qualification-only audio milestones to the AmigaChrome backend.

The runtime gate already proves SDL audio opens.  These markers go further:
they require an Ogg asset to decode into a Mix_Chunk, a mixer channel to start
and report active, and the audio subsystem to close during a clean game exit.
They are applied only when ASTROMENACE_DIAGNOSTICS=1.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
audio_cpp = source / "src" / "core" / "audio" / "audio_amigachrome.cpp"
text = audio_cpp.read_text(encoding="utf-8")

if "stage-audio-channel-active" in text:
    print("AstroMenace audio runtime probe already present")
    raise SystemExit(0)

include_anchor = '#include "SDL_mixer.h"\n'
include_insert = '''#include "SDL_mixer.h"
#ifdef AMIGACHROME
#include <proto/dos.h>
#include <dos/dos.h>
#endif
'''
if include_anchor not in text:
    raise RuntimeError("AstroMenace audio SDL_mixer include marker missing")
text = text.replace(include_anchor, include_insert, 1)

namespace_anchor = '''namespace viewizard {
namespace {

'''
namespace_insert = '''namespace viewizard {
namespace {

#ifdef AMIGACHROME
static bool AMAudioProbeEnabled()
{
    BPTR File = Open((CONST_STRPTR)"PROGDIR:AUTOTEST_FIRST_MISSION", MODE_OLDFILE);
    if (!File) return false;
    Close(File);
    return true;
}

static void AMAudioProbeStage(const char *Name)
{
    if (!AMAudioProbeEnabled()) return;
    BPTR File = Open((CONST_STRPTR)Name, MODE_NEWFILE);
    if (File) {
        static const char Ready[] = "ready\\n";
        Write(File, Ready, sizeof(Ready) - 1);
        Close(File);
    }
}

bool AudioProbeActiveMarked = false;
#endif

'''
if namespace_anchor not in text:
    raise RuntimeError("AstroMenace audio namespace marker missing")
text = text.replace(namespace_anchor, namespace_insert, 1)

open_anchor = '''    Mix_AllocateChannels(64);
    AudioReady = true;
    std::cout << "Audio      : AmigaChrome OpenGPU SDL2_mixer\\n";
'''
open_insert = '''    Mix_AllocateChannels(64);
    AudioReady = true;
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-open");
#endif
    std::cout << "Audio      : AmigaChrome OpenGPU SDL2_mixer\\n";
'''
if open_anchor not in text:
    raise RuntimeError("AstroMenace audio-open marker missing")
text = text.replace(open_anchor, open_insert, 1)

play_anchor = '''    m.chunk = chunk;
    m.channel = channel;
    Mix_Volume(channel, mixVolume(m.localVolume * m.globalVolume));
    return true;
'''
play_insert = '''    m.chunk = chunk;
    m.channel = channel;
    Mix_Volume(channel, mixVolume(m.localVolume * m.globalVolume));
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-channel-started");
    if (Mix_Playing(channel)) {
        AMAudioProbeStage("PROGDIR:stage-audio-channel-active");
        AudioProbeActiveMarked = true;
    }
#endif
    return true;
'''
if play_anchor not in text:
    raise RuntimeError("AstroMenace music-channel start marker missing")
text = text.replace(play_anchor, play_insert, 1)

playing_anchor = '''bool vw_IsAnyMusicPlaying()
{
    for (auto &p : Music)
        if (p.second.channel >= 0 && Mix_Playing(p.second.channel))
            return true;
    return false;
}
'''
playing_insert = '''bool vw_IsAnyMusicPlaying()
{
    for (auto &p : Music)
        if (p.second.channel >= 0 && Mix_Playing(p.second.channel)) {
#ifdef AMIGACHROME
            if (!AudioProbeActiveMarked) {
                AMAudioProbeStage("PROGDIR:stage-audio-channel-active");
                AudioProbeActiveMarked = true;
            }
#endif
            return true;
        }
    return false;
}
'''
if playing_anchor not in text:
    raise RuntimeError("AstroMenace any-music-playing marker missing")
text = text.replace(playing_anchor, playing_insert, 1)

shutdown_anchor = '''    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    AudioReady = false;
}
'''
shutdown_insert = '''    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    AudioReady = false;
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-shutdown");
#endif
}
'''
if shutdown_anchor not in text:
    raise RuntimeError("AstroMenace audio-shutdown marker missing")
text = text.replace(shutdown_anchor, shutdown_insert, 1)

audio_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace audio runtime qualification milestones")
