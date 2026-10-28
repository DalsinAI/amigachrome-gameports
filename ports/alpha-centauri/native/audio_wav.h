/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portable RIFF/WAVE reader for the in-game audio service (WP10).
 * No SDL, no allocation, explicit little-endian byte reads (safe on 68040).
 */
#ifndef ALPHA_AUDIO_WAV_H
#define ALPHA_AUDIO_WAV_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
enum class WavCodec : uint8_t { pcm_u8, pcm_s16, ima_adpcm, ms_adpcm };
enum class WavError : uint8_t { ok, truncated, not_riff, bad_chunk, bad_format, unsupported, too_large, capacity, no_data };
const char* wav_error_name(WavError);
struct WavInfo {
 WavCodec codec;
 uint16_t channels;        // 1 or 2; stereo is averaged to mono on decode
 uint32_t rate;            // 4000..48000 Hz
 uint16_t block_align;
 uint16_t samples_per_block; // ADPCM only
 uint16_t coefficient_count; // MS ADPCM only
 int16_t coefficients[32][2];// MS ADPCM only
 size_t data_offset;       // from the start of the file
 size_t data_bytes;        // whole frames/blocks only, clamped to the file
 uint32_t frames;          // mono frames produced by a full decode
 bool clamped;             // data chunk claimed more bytes than the file holds
};
// Parse a complete file or (for streaming) a prefix of `file_size` bytes that
// contains every chunk header up to and including the data chunk header.
// Unknown chunks and odd padding are skipped; duplicate fmt/data are rejected.
WavError parse_wav(const uint8_t* bytes, size_t available, size_t file_size, WavInfo& info);
// Decode a complete in-memory file to signed native-endian mono samples.
// Transactional: on failure `out` and `count` are untouched.
WavError decode_wav_mono(const uint8_t* bytes, size_t size, const WavInfo& info,
                         int16_t* out, size_t capacity, size_t& count);
// Streaming helper for PCM codecs: convert `frames` whole frames of raw data.
// Returns false for ADPCM (music tracks are expected as PCM, see AUDIO.md).
bool convert_pcm_frames(const uint8_t* raw, size_t frames, const WavInfo& info, int16_t* out);
inline size_t wav_frame_bytes(const WavInfo& i){return i.codec==WavCodec::pcm_u8?i.channels:i.codec==WavCodec::pcm_s16?2u*i.channels:0;}
}
#endif
