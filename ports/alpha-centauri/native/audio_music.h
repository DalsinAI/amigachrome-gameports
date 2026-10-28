/* SPDX-License-Identifier: GPL-3.0-or-later
 * Streaming music producer for the WP10 mixer (stdio only, no SDL).
 */
#ifndef ALPHA_AUDIO_MUSIC_H
#define ALPHA_AUDIO_MUSIC_H
#include "audio_events.h"
#include "audio_mixer.h"
#include "audio_wav.h"
#include <stdio.h>
namespace alpha {
class MusicStreamer {
public:
 ~MusicStreamer(){close();}
 // Opens a PCM WAV for streaming. ADPCM music is rejected (convert it, AUDIO.md).
 WavError open(const char* path);
 void close();
 size_t read(int16_t* out, size_t max_frames); // 0 once the data chunk is exhausted
 bool active() const {return file_!=nullptr;}
 bool at_end() const {return file_ && !left_;}
 uint32_t rate() const {return info_.rate;}
private:
 FILE* file_=nullptr;
 WavInfo info_{};
 size_t left_=0; // frames
 uint8_t raw_[4096];
};
struct MusicPump {
 MusicPlaylist playlist;
 MusicStreamer stream;
 char directory[160]=""; // prefix for playlist paths ("" = current directory)
 uint32_t tracks_started=0, failures=0;
 bool stopped=false;      // every playlist entry failed to open
 bool waiting=false;      // opened, but the previous track (other rate) is still draining
 int16_t scratch[2048];
};
typedef void (*AudioLockFn)(void* context);
// One producer step: open the next track once the ring has drained, or top the
// ring up. Every mixer access is wrapped in lock/unlock (SDL_LockAudioDevice);
// file I/O happens outside the lock. Returns frames pushed.
size_t music_pump(MusicPump& pump, AudioMixer& mixer, AudioLockFn lock, AudioLockFn unlock, void* context);
bool join_audio_path(const char* directory, const char* relative, char* out, size_t capacity);
}
#endif
