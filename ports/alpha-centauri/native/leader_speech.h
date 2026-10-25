/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LEADER_SPEECH_H
#define ALPHA_LEADER_SPEECH_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
struct LeaderSpeech { const char* stem; const char* leader; const char* faction; bool expansion; };
// Named faction selection speeches. No claim of dynamic diplomatic dialogue.
const LeaderSpeech* leader_speech(size_t index);
size_t leader_speech_count(bool expansion);
// Exact case-insensitive original faction basename, not a path. Unknown -> -1.
int find_leader_speech(const char* stem,bool expansion);
// Reads canonical RIFF PCM16LE mono22050 into host signed samples. Unknown chunks
// and odd padding allowed. Strict lengths/format, duplicate chunks rejected.
// No allocation; reports count only after fully validating. Overlap unsupported.
bool decode_speech_wav(const uint8_t* bytes,size_t size,int16_t* samples,
                       size_t capacity,size_t& count);
}
#endif
