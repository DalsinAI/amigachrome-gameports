/* SPDX-License-Identifier: GPL-3.0-or-later
 * Single-device software mixer for the in-game audio service (WP10).
 *
 * The native SDL backend cannot keep two audio devices open (see
 * TECHNOLOGY-BROWSER.md), so effects, voice and music share one 22050 Hz mono
 * S16 stream. Integer-only: no FPU work in the audio callback on 68040.
 * Not thread-safe by itself; the SDL service calls it under SDL_LockAudioDevice.
 */
#ifndef ALPHA_AUDIO_MIXER_H
#define ALPHA_AUDIO_MIXER_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
struct AudioClip { const int16_t* samples; uint32_t frames; uint32_t rate; };
struct MixerLevels {
 // 0..100 percent; a disabled bus is silent regardless of its volume.
 uint8_t master=100, sfx=100, voice=100, music=70;
 bool sfx_on=true, voice_on=true, music_on=true;
 uint8_t music_duck=40; // music level (percent) while a voice line plays
};
struct MixerStats { uint32_t rendered=0, music_underruns=0, clipped=0, sfx_stolen=0; };
class AudioMixer {
public:
 static const uint32_t output_rate=22050;
 static const int sfx_slots=6;
 static const size_t music_ring=32768; // 1.48 s at 22050 Hz
 void reset();
 void set_levels(const MixerLevels& levels){levels_=levels;}
 const MixerLevels& levels() const {return levels_;}
 // Returns the slot used; steals the oldest effect when all slots are busy.
 int play_sfx(const AudioClip& clip, uint8_t gain_percent=100);
 void stop_sfx();
 void play_voice(const AudioClip& clip);
 void stop_voice();
 // Linear fade of the current voice line; it stops when the fade completes.
 void fade_voice(uint32_t milliseconds);
 bool voice_active() const {return voice_.clip.samples!=nullptr;}
 bool sfx_active() const;
 // True while any channel still reads `samples` (eviction guard).
 bool references(const int16_t* samples) const;
 // Music is a ring of source-rate samples written by the streaming producer.
 void music_start(uint32_t rate);
 void music_stop();
 size_t music_space() const {return music_ring-1-music_fill();}
 size_t music_fill() const {return (music_write_+music_ring-music_read_)%music_ring;}
 size_t music_push(const int16_t* samples, size_t count);
 void music_end_of_stream(){music_eos_=true;}
 // The next track continues the same ring without a gap (same source rate).
 void music_continue(){music_eos_=false;}
 uint32_t music_rate_step() const {return music_step_;}
 bool music_live() const {return music_live_;}
 static uint32_t step_for(uint32_t rate);
 // True once the producer signalled the end and the ring has drained.
 bool music_finished() const {return music_eos_ && !music_fill();}
 void render(int16_t* out, size_t frames);
 const MixerStats& stats() const {return stats_;}
private:
 struct Channel { AudioClip clip{}; uint32_t pos=0, frac=0, step=1u<<16; int32_t gain=0; uint32_t serial=0; int32_t fade=0, fade_step=0; };
 static bool mix_channel(Channel& c, int32_t* acc, size_t frames, int32_t gain);
 int32_t bus_gain(uint8_t bus, bool on) const;
 Channel sfx_[sfx_slots];
 Channel voice_;
 uint32_t serial_=0;
 int16_t music_[music_ring];
 size_t music_read_=0, music_write_=0;
 uint32_t music_frac_=0, music_step_=1u<<16;
 bool music_live_=false, music_eos_=false;
 int32_t music_gain_now_=0; // Q16, ramps toward its target to avoid clicks
 int32_t acc_[512];
 MixerLevels levels_{};
 MixerStats stats_{};
};
}
#endif
