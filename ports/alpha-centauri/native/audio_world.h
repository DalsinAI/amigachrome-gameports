/* SPDX-License-Identifier: GPL-3.0-or-later
 * World-view glue for the WP10 audio service (keeps original_map_window.cpp
 * edits to single calls).
 */
#ifndef ALPHA_AUDIO_WORLD_H
#define ALPHA_AUDIO_WORLD_H
#include "audio_service.h"
#include "original_world.h"
namespace alpha {
// Opens the service in the current (original data) directory. Settings come
// from alpha-audio.cfg when present, otherwise from the save's original
// sound/music/voice preference bits. Starts the human faction's music
// playlist and plays the turn-start event.
void audio_world_enter(const OriginalWorld& world);
// Saves alpha-audio.cfg if a key toggle changed it, then closes the service.
void audio_world_leave();
// World keys: 'm' music on/off, 'n' sound effects on/off, 'j' voices on/off.
// Returns a status line, or nullptr when the key is not an audio key.
const char* audio_world_key(unsigned code);
// Lower-case original faction file stem ("gaians") of faction `index`.
bool audio_faction_stem(const OriginalWorld& world, size_t index, char* out, size_t capacity);
}
#endif
