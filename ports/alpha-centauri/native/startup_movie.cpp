/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "startup_movie.h"
#include <string.h>
namespace alpha {
static uint32_t le(const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
bool startup_movie_header(const uint8_t* p,size_t n,uint64_t bytes,StartupMovieInfo& out){
 if(!p || n!=40 || memcmp(p,"AMOVYUV1",8))return false;
 StartupMovieInfo m;m.width=le(p+8);m.height=le(p+12);m.frames=le(p+16);m.rate=le(p+20);m.channels=le(p+24);m.samples_per_frame=le(p+28);m.audio_frames=le(p+32);
 if(le(p+36) || m.width<2 || m.height<2 || m.width>640 || m.height>480 || (m.width&1) || (m.height&1) || !m.frames || m.frames>18000 || m.rate!=22050 || m.channels!=2 || m.samples_per_frame!=1470)return false;
 /* A/V tails may differ by less than one frame; never invent a missing scene. */
 uint64_t capacity=uint64_t(m.frames)*1470;
 if(!m.audio_frames || uint64_t(m.audio_frames)>=capacity+1470 || uint64_t(m.audio_frames)+1470<=capacity)return false;
 m.video_bytes=size_t(m.width)*m.height*3/2;m.max_record_bytes=m.video_bytes+2940*4;
 m.file_bytes=40+uint64_t(m.frames)*m.video_bytes+uint64_t(m.audio_frames)*4;
 if(m.file_bytes!=bytes)return false;
 out=m;return true;
}
uint32_t startup_movie_audio_frames(const StartupMovieInfo& m,uint32_t f){
 if(f>=m.frames)return 0;
 uint64_t used=uint64_t(f)*m.samples_per_frame;
 if(used>=m.audio_frames)return 0;
 uint32_t left=m.audio_frames-uint32_t(used);return f+1==m.frames?left:(left<m.samples_per_frame?left:m.samples_per_frame);
}
static uint8_t clamp(int x){return uint8_t(x<0?0:x>255?255:x);}
bool startup_movie_rgb(const StartupMovieInfo& m,const uint8_t* p,size_t n,uint8_t* out,size_t count){
 if(!p || !out || m.width<2 || m.height<2 || m.width>640 || m.height>480 || (m.width&1) || (m.height&1))return false;
 size_t area=size_t(m.width)*m.height;if(n!=area*3/2 || count!=area*3)return false;
 uintptr_t a=reinterpret_cast<uintptr_t>(p),b=reinterpret_cast<uintptr_t>(out);
 if(a>UINTPTR_MAX-n || b>UINTPTR_MAX-count || (a<b+count && b<a+n))return false;
 for(uint32_t y=0;y<m.height;++y)for(uint32_t x=0;x<m.width;++x){size_t i=size_t(y)*m.width+x,j=size_t(y/2)*(m.width/2)+x/2;int yy=int(p[i])-16,u=int(p[area+j])-128,v=int(p[area+area/4+j])-128;
  out[i*3]=clamp((298*yy+409*v+128)>>8);out[i*3+1]=clamp((298*yy-100*u-208*v+128)>>8);out[i*3+2]=clamp((298*yy+516*u+128)>>8);
 }return true;
}
}
