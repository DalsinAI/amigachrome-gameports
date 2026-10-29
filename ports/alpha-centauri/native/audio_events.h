/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP10 event catalog, original media names, settings and music playlist.
 * Portable (no SDL); the SDL service in audio_service.cpp consumes it.
 */
#ifndef ALPHA_AUDIO_EVENTS_H
#define ALPHA_AUDIO_EVENTS_H
#include "audio_mixer.h"
#include <stddef.h>
#include <stdint.h>
namespace alpha {
enum class AudioEvent : uint8_t {
 ui_click, menu_open, menu_close, panel_slide, command_menu, quit_prompt,
 unit_move, unit_attack, combat_hit, psi_attack, unit_destroyed, psi_destroyed,
 base_built, facility_complete, tech_discovered, turn_start, turn_complete,
 count
};
const size_t audio_event_count=(size_t)AudioEvent::count;
struct AudioEventInfo {
 const char* key;       // settings key suffix, e.g. "unit_move"
 const char* file;      // default original fx basename, or nullptr (unmapped)
 const char* source;    // where the default name is attested
};
const AudioEventInfo& audio_event_info(AudioEvent);
bool audio_event_from_key(const char* key, size_t length, AudioEvent& out);

// Settings: a small text file of key=value lines (see AUDIO.md).
const size_t audio_name_bytes=64;
struct AudioSettings {
 MixerLevels levels{};
 bool voice_stops_on_close=true; // original MPREF_AV_VOICEOVER_STOP_CLOSE_POPUP
 bool mapped[audio_event_count];  // false = use the catalog default
 char file[audio_event_count][audio_name_bytes]; // "" with mapped=true = silent
 AudioSettings();
};
// Returns 0 on success or the 1-based line number of the first bad line;
// `settings` is changed only on success.
unsigned parse_audio_settings(const char* text, size_t length, AudioSettings& settings);
// Writes a complete settings file; returns bytes written or 0 if it did not fit.
size_t format_audio_settings(const AudioSettings& settings, char* out, size_t capacity);
// Original Alpha Centauri.ini preference bits (Thinker engine_enums.h):
// PREF_AV_VOLUME_SFX_TOGGLE 0x400, PREF_AV_VOLUME_MUSIC_TOGGLE 0x800,
// MPREF_AV_VOLUME_VOICE_TOGGLE 0x4000, MPREF_AV_VOICEOVER_STOP_CLOSE_POPUP 0x20000.
void apply_original_preferences(uint32_t preferences, uint32_t more_preferences, AudioSettings& settings);
// The fx basename an event plays under `settings`, or nullptr when silent.
const char* audio_event_file(const AudioSettings& settings, AudioEvent);

// Original voice-over paths (pre-converted to .wav, see AUDIO.md). Each
// returns false for an invalid id and writes nothing in that case.
bool faction_voice_path(const char* faction_stem, bool expansion, char* out, size_t capacity);
bool technology_voice_path(int technology_id, char* out, size_t capacity);
bool facility_voice_path(int facility_id, char* out, size_t capacity);

// In-game music. The original plays AMB score sets chosen per faction
// (Thinker game.cpp load_music defaults). Without an AMB sequencer the
// fallback plays owner-prepared PCM renders music/<set>.wav, music/<set>-N.wav.
const char* faction_music_set(const char* faction_stem);
size_t music_set_count();
const char* music_set(size_t index);
typedef bool (*AudioExistsFn)(void* context, const char* path);
struct MusicPlaylist {
 static const size_t max_tracks=32;
 char track[max_tracks][audio_name_bytes];
 size_t count=0, cursor=0;
 // Faction set first; otherwise the default set; otherwise every other set.
 size_t build(const char* faction_stem, AudioExistsFn exists, void* context);
 const char* next(); // cycles; nullptr when empty
};
}
#endif
