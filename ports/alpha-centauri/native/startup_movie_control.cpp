/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "startup_movie.h"
#include <stdio.h>
#include <string.h>
static void le(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=uint8_t(n>>(i*8));}
int main(){
 uint8_t header[40]{};memcpy(header,"AMOVYUV1",8);uint32_t fields[]={4,4,2,22050,2,1470,2940,0};for(unsigned i=0;i<8;++i)le(header+8+i*4,fields[i]);
 alpha::StartupMovieInfo movie;unsigned cases=0;
 if(!alpha::startup_movie_header(header,40,40+48+11760,movie))return 20;
 ++cases;
 if(movie.width!=4 || movie.height!=4 || movie.frames!=2 || movie.video_bytes!=24 || alpha::startup_movie_audio_frames(movie,0)!=1470 || alpha::startup_movie_audio_frames(movie,1)!=1470 || alpha::startup_movie_audio_frames(movie,2))return 20;
 ++cases;
 const struct {unsigned offset;uint32_t value;} bad[]={{0,0},{8,0},{8,1},{8,641},{12,481},{12,3},{16,0},{16,18001},{20,44100},{24,1},{28,1471},{32,0},{32,1470},{32,4410},{36,1}};
 for(const auto& b:bad){uint8_t copy[40];memcpy(copy,header,40);le(copy+b.offset,b.value);alpha::StartupMovieInfo old=movie;if(alpha::startup_movie_header(copy,40,movie.file_bytes,old) || old.width!=movie.width || old.audio_frames!=movie.audio_frames)return 20;
 ++cases;}
 if(alpha::startup_movie_header(nullptr,40,movie.file_bytes,movie) || alpha::startup_movie_header(header,39,movie.file_bytes,movie) || alpha::startup_movie_header(header,40,movie.file_bytes+1,movie))return 20;
 ++cases;
 const uint32_t tails[]={2940-1469,2940+1469};
 for(uint32_t count: tails){le(header+32,count);if(!alpha::startup_movie_header(header,40,40+48+uint64_t(count)*4,movie) || alpha::startup_movie_audio_frames(movie,0)+alpha::startup_movie_audio_frames(movie,1)!=count)return 20;
 ++cases;}
 uint8_t yuv[24],rgb[48];memset(yuv,128,sizeof(yuv));memset(yuv,16,16);if(!alpha::startup_movie_rgb(movie,yuv,24,rgb,48))return 20;for(auto v:rgb)if(v)return 20;
 ++cases;
 if(alpha::startup_movie_rgb(movie,yuv,23,rgb,48) || alpha::startup_movie_rgb(movie,yuv,24,rgb,47) || alpha::startup_movie_rgb(movie,yuv,24,yuv,48) || alpha::startup_movie_rgb(movie,nullptr,24,rgb,48))return 20;
 ++cases;
 printf("STARTUP CONTROL cases=%u RC 0\n",cases);return 0;
}
