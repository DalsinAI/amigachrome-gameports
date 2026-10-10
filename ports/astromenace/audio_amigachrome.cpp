/*
 * AstroMenace audio backend for AmigaChrome.
 * Replaces the upstream OpenAL/Vorbis implementation with the OpenGPU
 * SDL2_mixer satellite. Ogg/Vorbis decoding therefore follows the normal
 * OpenGPU media.decode -> CPU fallback ladder and output follows SDL2's
 * Amiga audio backend.
 */
#include "audio.h"
#include "../vfs/vfs.h"
#include "../math/math.h"
#include "SDL.h"
#include "SDL_mixer.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace viewizard {
namespace {

bool AudioReady = false;
float ListenerX = 0.0f;
float ListenerY = 0.0f;
float ListenerZ = 0.0f;

struct BufferEntry {
    unsigned id;
    Mix_Chunk *chunk;
};

struct SoundState {
    std::string name;
    int channel;
    float localVolume;
    float globalVolume;
    bool allowedStop;
    bool relative;
    int attenuationType;
    float x, y, z;
    uint32_t lastTick;
    uint32_t destroyPeriod;
    uint32_t destroyTicks;
};

struct MusicState {
    Mix_Chunk *chunk;
    int channel;
    float localVolume;
    float globalVolume;
    bool looped;
    std::string loopPart;
    bool fadeIn;
    bool fadeOut;
    float fadeStart;
    float fadeEnd;
    uint32_t fadeTicks;
    uint32_t fadePeriod;
    uint32_t lastTick;
};

std::unordered_map<std::string, BufferEntry> SoundBuffers;
std::unordered_map<unsigned, SoundState> Sounds;
std::unordered_map<std::string, MusicState> Music;
unsigned NextBufferId = 1;
unsigned NextSoundId = 1;

static int mixVolume(float v)
{
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return static_cast<int>(v * static_cast<float>(MIX_MAX_VOLUME));
}

static Mix_Chunk *loadChunk(const std::string &name)
{
    std::unique_ptr<cFILE> file = vw_fopen(name);
    if (!file || file->GetSize() <= 0 || file->GetData() == nullptr)
        return nullptr;

    SDL_RWops *rw = SDL_RWFromConstMem(file->GetData(), static_cast<int>(file->GetSize()));
    if (!rw)
        return nullptr;

    /* Mix_LoadWAV_RW is intentionally used for both WAV and Ogg. OpenGPU's
       SDL2_mixer satellite advertises Ogg through this path and can service
       it via media.decode before falling back to its bundled stb_vorbis. */
    return Mix_LoadWAV_RW(rw, 1);
}

static BufferEntry *bufferFor(const std::string &name)
{
    auto it = SoundBuffers.find(name);
    if (it != SoundBuffers.end())
        return &it->second;

    Mix_Chunk *chunk = loadChunk(name);
    if (!chunk)
        return nullptr;

    BufferEntry entry{NextBufferId++, chunk};
    auto result = SoundBuffers.emplace(name, entry);
    return &result.first->second;
}

static void applySpatial(SoundState &s)
{
    if (s.channel < 0)
        return;

    float dx = s.relative ? s.x : (s.x - ListenerX);
    float dy = s.relative ? s.y : (s.y - ListenerY);
    float dz = s.relative ? s.z : (s.z - ListenerZ);
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    float maxDistance = s.attenuationType == 1 ? 250.0f :
                        (s.attenuationType == 2 ? 600.0f : 1200.0f);
    float attenuation = 1.0f - distance / maxDistance;
    if (attenuation < 0.08f) attenuation = 0.08f;
    if (attenuation > 1.0f) attenuation = 1.0f;

    Mix_Volume(s.channel, mixVolume(s.localVolume * s.globalVolume * attenuation));

    float pan = dx / 220.0f;
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    Uint8 left = static_cast<Uint8>(255.0f * (pan > 0.0f ? 1.0f - pan : 1.0f));
    Uint8 right = static_cast<Uint8>(255.0f * (pan < 0.0f ? 1.0f + pan : 1.0f));
    Mix_SetPanning(s.channel, left, right);
}

static unsigned newSoundId()
{
    do {
        ++NextSoundId;
        if (NextSoundId == 0) ++NextSoundId;
    } while (Sounds.find(NextSoundId) != Sounds.end());
    return NextSoundId;
}

static void freeMusic(MusicState &m)
{
    if (m.channel >= 0)
        Mix_HaltChannel(m.channel);
    if (m.chunk)
        Mix_FreeChunk(m.chunk);
    m.chunk = nullptr;
    m.channel = -1;
}

static bool startMusicChunk(MusicState &m, const std::string &name, int loops)
{
    Mix_Chunk *chunk = loadChunk(name);
    if (!chunk)
        return false;
    int channel = Mix_PlayChannel(-1, chunk, loops);
    if (channel < 0) {
        Mix_FreeChunk(chunk);
        return false;
    }
    if (m.chunk)
        Mix_FreeChunk(m.chunk);
    m.chunk = chunk;
    m.channel = channel;
    Mix_Volume(channel, mixVolume(m.localVolume * m.globalVolume));
    return true;
}

} // namespace

bool vw_InitAudio()
{
    if (AudioReady)
        return true;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::cerr << "vw_InitAudio(): SDL audio init failed: " << SDL_GetError() << "\n";
        return false;
    }
    const int wanted = MIX_INIT_OGG;
    const int got = Mix_Init(wanted);
    if ((got & wanted) != wanted) {
        std::cerr << "vw_InitAudio(): SDL2_mixer Ogg decoder unavailable: " << Mix_GetError() << "\n";
        Mix_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    if (Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 1024) != 0) {
        std::cerr << "vw_InitAudio(): Mix_OpenAudio failed: " << Mix_GetError() << "\n";
        Mix_Quit();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    Mix_AllocateChannels(64);
    AudioReady = true;
    std::cout << "Audio      : AmigaChrome OpenGPU SDL2_mixer\n";
    return true;
}

bool vw_GetAudioStatus()
{
    return AudioReady;
}

void vw_ShutdownAudio()
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

void vw_Listener(float (&position)[3], float (&)[3], float (&)[6])
{
    ListenerX = position[0];
    ListenerY = position[1];
    ListenerZ = position[2];
    for (auto &p : Sounds)
        applySpatial(p.second);
}

unsigned int vw_LoadSoundBuffer(const std::string &name)
{
    BufferEntry *entry = bufferFor(name);
    return entry ? entry->id : 0;
}

void vw_ReleaseSoundBuffer(const std::string &name)
{
    for (auto it = Sounds.begin(); it != Sounds.end();) {
        if (it->second.name == name) {
            if (it->second.channel >= 0) Mix_HaltChannel(it->second.channel);
            it = Sounds.erase(it);
        } else {
            ++it;
        }
    }
    auto it = SoundBuffers.find(name);
    if (it != SoundBuffers.end()) {
        if (it->second.chunk) Mix_FreeChunk(it->second.chunk);
        SoundBuffers.erase(it);
    }
}

unsigned int vw_PlaySound(const std::string &name, float localVolume, float globalVolume,
                          const sVECTOR3D &location, bool relative, bool allowStop, int atType)
{
    BufferEntry *entry = bufferFor(name);
    if (!AudioReady || !entry || !entry->chunk)
        return 0;
    int channel = Mix_PlayChannel(-1, entry->chunk, 0);
    if (channel < 0)
        return 0;

    unsigned id = newSoundId();
    SoundState state{name, channel, localVolume, globalVolume, allowStop, relative, atType,
                     location.x, location.y, location.z, SDL_GetTicks(), 0, 0};
    Sounds.emplace(id, state);
    applySpatial(Sounds[id]);
    return id;
}

bool vw_IsSoundAvailable(unsigned int id)
{
    return id != 0 && Sounds.find(id) != Sounds.end();
}

unsigned int vw_ReplayFirstFoundSound(const std::string &name)
{
    for (auto &p : Sounds) {
        if (p.second.name == name) {
            BufferEntry *entry = bufferFor(name);
            if (!entry) return 0;
            int channel = Mix_PlayChannel(-1, entry->chunk, 0);
            if (channel < 0) return 0;
            p.second.channel = channel;
            p.second.lastTick = SDL_GetTicks();
            p.second.destroyPeriod = 0;
            p.second.destroyTicks = 0;
            applySpatial(p.second);
            return p.first;
        }
    }
    return 0;
}

void vw_SetSoundGlobalVolume(const std::string &name, float volume)
{
    for (auto &p : Sounds)
        if (p.second.name == name) {
            p.second.globalVolume = volume;
            applySpatial(p.second);
        }
}

void vw_SetSoundLocation(unsigned int id, float x, float y, float z)
{
    auto it = Sounds.find(id);
    if (it == Sounds.end()) return;
    it->second.x = x; it->second.y = y; it->second.z = z;
    applySpatial(it->second);
}

void vw_StopSound(unsigned int id, uint32_t delay)
{
    auto it = Sounds.find(id);
    if (it == Sounds.end()) return;
    if (delay == 0) {
        Mix_HaltChannel(it->second.channel);
    } else {
        it->second.destroyPeriod = delay;
        it->second.destroyTicks = 0;
    }
}

void vw_UpdateSound(uint32_t now)
{
    for (auto it = Sounds.begin(); it != Sounds.end();) {
        SoundState &s = it->second;
        uint32_t delta = now - s.lastTick;
        s.lastTick = now;
        bool erase = false;
        if (s.destroyPeriod) {
            s.destroyTicks += delta;
            if (s.destroyTicks >= s.destroyPeriod) {
                Mix_HaltChannel(s.channel);
                erase = true;
            } else {
                float remain = 1.0f - static_cast<float>(s.destroyTicks) /
                                       static_cast<float>(s.destroyPeriod);
                float old = s.localVolume;
                s.localVolume *= remain;
                applySpatial(s);
                s.localVolume = old;
            }
        }
        if (!erase && s.channel >= 0 && !Mix_Playing(s.channel))
            erase = true;
        if (erase) it = Sounds.erase(it); else ++it;
    }
}

void vw_ReleaseAllSounds()
{
    for (auto &p : Sounds)
        if (p.second.channel >= 0) Mix_HaltChannel(p.second.channel);
    Sounds.clear();
}

void vw_StopAllSoundsIfAllowed()
{
    for (auto it = Sounds.begin(); it != Sounds.end();) {
        if (it->second.allowedStop) {
            if (it->second.channel >= 0) Mix_HaltChannel(it->second.channel);
            it = Sounds.erase(it);
        } else {
            ++it;
        }
    }
}

bool vw_PlayMusic(const std::string &name, float localVolume, float globalVolume,
                  bool loop, const std::string &loopFileName)
{
    if (!AudioReady || name.empty())
        return false;
    if (Music.find(name) != Music.end())
        return true;

    MusicState m{nullptr, -1, localVolume, globalVolume, loop, loopFileName,
                 false, false, localVolume, localVolume, 0, 0, SDL_GetTicks()};
    if (!startMusicChunk(m, name, loop ? -1 : 0))
        return false;
    Music.emplace(name, m);
    return true;
}

void vw_SetMusicGlobalVolume(float volume)
{
    for (auto &p : Music) {
        p.second.globalVolume = volume;
        if (p.second.channel >= 0)
            Mix_Volume(p.second.channel, mixVolume(p.second.localVolume * volume));
    }
}

bool vw_IsMusicPlaying(const std::string &name)
{
    auto it = Music.find(name);
    return it != Music.end() && it->second.channel >= 0 && Mix_Playing(it->second.channel);
}

bool vw_IsAnyMusicPlaying()
{
    for (auto &p : Music)
        if (p.second.channel >= 0 && Mix_Playing(p.second.channel))
            return true;
    return false;
}

void vw_FadeOutAllMusicWithException(const std::string &name, uint32_t ticks,
                                     float exceptionEnd, uint32_t exceptionTicks)
{
    for (auto &p : Music) {
        MusicState &m = p.second;
        if (p.first == name) {
            if (m.fadeOut && exceptionEnd > 0.0f) {
                m.fadeOut = false;
                m.fadeIn = true;
                m.fadeStart = m.localVolume;
                m.fadeEnd = exceptionEnd;
                m.fadeTicks = 0;
                m.fadePeriod = exceptionTicks ? exceptionTicks : 1;
            }
        } else if (!m.fadeOut) {
            m.fadeIn = false;
            m.fadeOut = true;
            m.fadeStart = m.localVolume;
            m.fadeEnd = 0.0f;
            m.fadeTicks = 0;
            m.fadePeriod = ticks ? ticks : 1;
        }
    }
}

void vw_SetMusicFadeIn(const std::string &name, float endVolume, uint32_t ticks)
{
    auto it = Music.find(name);
    if (it == Music.end() || it->second.fadeIn) return;
    MusicState &m = it->second;
    m.fadeOut = false;
    m.fadeIn = true;
    m.fadeStart = m.localVolume;
    m.fadeEnd = endVolume;
    m.fadeTicks = 0;
    m.fadePeriod = ticks ? ticks : 1;
    m.lastTick = SDL_GetTicks();
}

void vw_UpdateMusic(uint32_t now)
{
    for (auto it = Music.begin(); it != Music.end();) {
        MusicState &m = it->second;
        uint32_t delta = now - m.lastTick;
        m.lastTick = now;
        bool erase = false;

        if (m.fadeIn || m.fadeOut) {
            m.fadeTicks += delta;
            float t = static_cast<float>(m.fadeTicks) / static_cast<float>(m.fadePeriod);
            if (t > 1.0f) t = 1.0f;
            m.localVolume = m.fadeStart + (m.fadeEnd - m.fadeStart) * t;
            if (m.channel >= 0)
                Mix_Volume(m.channel, mixVolume(m.localVolume * m.globalVolume));
            if (t >= 1.0f) {
                if (m.fadeOut) {
                    Mix_HaltChannel(m.channel);
                    erase = true;
                }
                m.fadeIn = false;
                m.fadeOut = false;
            }
        }

        if (!erase && m.channel >= 0 && !Mix_Playing(m.channel)) {
            if (!m.loopPart.empty()) {
                std::string next = m.loopPart;
                m.loopPart.clear();
                m.looped = true;
                if (!startMusicChunk(m, next, -1))
                    erase = true;
            } else if (!m.looped) {
                erase = true;
            }
        }

        if (erase) {
            freeMusic(m);
            it = Music.erase(it);
        } else {
            ++it;
        }
    }
}

void vw_ReleaseMusic(const std::string &name)
{
    auto it = Music.find(name);
    if (it == Music.end()) return;
    freeMusic(it->second);
    Music.erase(it);
}

void vw_ReleaseAllMusic()
{
    for (auto &p : Music)
        freeMusic(p.second);
    Music.clear();
}

} // namespace viewizard
