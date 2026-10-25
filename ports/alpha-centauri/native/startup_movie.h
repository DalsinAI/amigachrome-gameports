/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_STARTUP_MOVIE_H
#define ALPHA_STARTUP_MOVIE_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
/* Installation cache of decoded owner-supplied WVE. This is NOT a native TQI
 * decoder. Fixed little-endian fields; no native structs are serialized.
 * Header: AMOVYUV1, width,height,frames,rate,channels,samples_per_frame,
 * total_audio_frames,reserved (eight uint32 fields). Each record contains
 * YUV420 planar video then min(samples_per_frame,remaining) stereo S16LE.
 * The original 15fps / 22050Hz timing is integral (1470 samples per frame).
 */
struct StartupMovieInfo {
 uint32_t width=0,height=0,frames=0,rate=0,channels=0,samples_per_frame=0,audio_frames=0;
 size_t video_bytes=0,max_record_bytes=0;uint64_t file_bytes=0;
};
bool startup_movie_header(const uint8_t* header,size_t length,uint64_t file_bytes,StartupMovieInfo& result);
uint32_t startup_movie_audio_frames(const StartupMovieInfo& movie,uint32_t frame);
/* BT.601 limited-range presentation of decoded YUV, without scaling/filtering.
 * Caller supplies non-overlapping, tightly packed output width*height*3. */
bool startup_movie_rgb(const StartupMovieInfo& movie,const uint8_t* yuv,size_t input_bytes,uint8_t* rgb,size_t output_bytes);
}
/* AmigaOS presentation API. Invoke before opening menu audio. Return 0 on
 * completion, 5 on user skip, 20 on invalid/unavailable playback. Build the
 * player with ALPHA_EMBEDDED_MOVIE to suppress its standalone main. */
int alpha_startup_movie(const char* cache_path);
#endif
