#!/usr/bin/env python3
"""Add qualification-only audio milestones to the AmigaChrome backend.

The runtime gate already proves SDL audio opens. These markers go further:
they require an Ogg asset to decode into a Mix_Chunk, a mixer channel to start
and report active, and every shutdown substep to complete. The patch itself is
applied only when ASTROMENACE_DIAGNOSTICS=1, so marker writes do not need to
depend on a second runtime sentinel lookup.
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
static void AMAudioProbeStage(const char *Name)
{
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

update_anchor = '''        uint32_t delta = now - m.lastTick;
        m.lastTick = now;
        bool erase = false;

        if (m.fadeIn || m.fadeOut) {
'''
update_insert = '''        uint32_t delta = now - m.lastTick;
        m.lastTick = now;
        bool erase = false;

#ifdef AMIGACHROME
        // Some SDL_mixer targets do not report the channel active until the
        // first mixer/update pass. Qualify actual playback here as well as at
        // Mix_PlayChannel return time.
        if (!AudioProbeActiveMarked &&
            m.channel >= 0 &&
            Mix_Playing(m.channel)) {
            AMAudioProbeStage("PROGDIR:stage-audio-channel-active");
            AudioProbeActiveMarked = true;
        }
#endif

        if (m.fadeIn || m.fadeOut) {
'''
if update_anchor not in text:
    raise RuntimeError("AstroMenace music update marker missing")
text = text.replace(update_anchor, update_insert, 1)

shutdown_anchor = '''void vw_ShutdownAudio()
{
    if (!AudioReady)
        return;
    vw_ReleaseAllSounds();
    vw_ReleaseAllMusic();
    for (auto &p : SoundBuffers)
        if (p.second.chunk) Mix_FreeChunk(p.second.chunk);
    SoundBuffers.clear();
    Mix_CloseAudio();
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    AudioReady = false;
}
'''
shutdown_insert = '''void vw_ShutdownAudio()
{
    if (!AudioReady) {
#ifdef AMIGACHROME
        AMAudioProbeStage("PROGDIR:stage-audio-shutdown-not-ready");
#endif
        return;
    }
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-before-release-sounds");
#endif
    vw_ReleaseAllSounds();
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-release-sounds");
    AMAudioProbeStage("PROGDIR:stage-audio-before-release-music");
#endif
    vw_ReleaseAllMusic();
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-release-music");
    AMAudioProbeStage("PROGDIR:stage-audio-before-buffer-free");
#endif
    for (auto &p : SoundBuffers)
        if (p.second.chunk) Mix_FreeChunk(p.second.chunk);
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-buffer-free");
#endif
    SoundBuffers.clear();
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-buffer-clear");
    AMAudioProbeStage("PROGDIR:stage-audio-before-mix-close");
#endif
    Mix_CloseAudio();
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-mix-close");
    AMAudioProbeStage("PROGDIR:stage-audio-before-mix-quit");
#endif
    Mix_Quit();
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-mix-quit");
    AMAudioProbeStage("PROGDIR:stage-audio-before-sdl-audio-quit");
#endif
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-after-sdl-audio-quit");
#endif
    AudioReady = false;
#ifdef AMIGACHROME
    AMAudioProbeStage("PROGDIR:stage-audio-shutdown");
#endif
}
'''
if shutdown_anchor not in text:
    raise RuntimeError("AstroMenace audio-shutdown function marker missing")
text = text.replace(shutdown_anchor, shutdown_insert, 1)

audio_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace audio runtime and shutdown qualification milestones")
