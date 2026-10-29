/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP10 in-game audio service: the one call surface the game uses by event.
 *
 * It owns the single SDL audio device for the world (effects, voice-over and
 * music mixed by AudioMixer). Every call is safe when the service is closed or
 * no device could be opened: the game then simply runs silent.
 * Original media are read from the game's data directory at run time.
 */
#ifndef ALPHA_AUDIO_SERVICE_H
#define ALPHA_AUDIO_SERVICE_H
#include "audio_events.h"
namespace alpha {
// data_directory: "" or nullptr = current directory. settings_path: the
// alpha-audio.cfg to load (missing file = defaults); nullptr skips it.
bool audio_open(const char* data_directory, const char* settings_path);
void audio_close();
bool audio_ready();      // a device is open
// Release the device while another component owns audio (menu loop, the
// technology browser's own voice device) and take it back afterwards; the
// music track position, playlist and settings survive.
void audio_suspend();
bool audio_resume();

// Sound effects by event (see AudioEvent / audio_event_info()).
void audio_event(AudioEvent event);
// Music: start the faction's playlist (faction file stem such as "gaians").
bool audio_world_music(const char* faction_stem);
void audio_music_stop();

// Voice-over hooks. Each returns true when a recording started.
bool audio_faction_speech(const char* faction_stem, bool expansion);
// NewTechWin_exec: played for the human discoverer only.
bool audio_tech_discovered(int technology_id, bool human_discoverer);
// Thinker base.cpp: voice only on the first announcement of an ordinary
// facility for the human player's own base; the effect plays regardless.
bool audio_facility_complete(int facility_id, bool human_first_announcement);
// Interludes are text-and-art popups: the current voice fades (2000 ms, as
// the original Sound_fade_2 calls) and music is held at the voice level.
void audio_interlude(int interlude_id);
void audio_interlude_closed();
// Popup closed: honours voice_stops_on_close (original MPREF 0x20000).
void audio_popup_closed();
void audio_stop_voice();

// Settings (volume and on/off). Changes apply immediately.
const AudioSettings& audio_settings();
void audio_set_settings(const AudioSettings& settings);
bool audio_save_settings(const char* path);

// Diagnostics for tests and the HUD.
struct AudioServiceStatus {
 bool ready, music_playing, voice_playing;
 unsigned cached_clips, missing_files, decode_failures, tracks_started, music_underruns;
 const char* last_error;
};
AudioServiceStatus audio_status();
}
#endif
