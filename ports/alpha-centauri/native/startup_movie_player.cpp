/* SPDX-License-Identifier: GPL-3.0-or-later
 * Bounded streaming presentation of a losslessly decoded installation cache.
 * Run before menu music opens its device. No original WVE decoder is claimed.
 */
#define SDL_MAIN_HANDLED
#include "startup_movie.h"
#include <SDL2/SDL.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern struct IntuitionBase* IntuitionBase;extern struct GfxBase* GfxBase;extern struct Library* CyberGfxBase;
#ifndef ALPHA_EMBEDDED_MOVIE
struct Library* CyberGfxBase;
#endif
namespace {
constexpr uint32_t RING=1470*8;
struct Audio {int16_t ring[RING*2];uint32_t read,write,available,played,starves;bool finishing;};
void sound(void* ptr,Uint8* stream,int bytes){
 auto& a=*static_cast<Audio*>(ptr);memset(stream,0,size_t(bytes));auto* out=reinterpret_cast<int16_t*>(stream);uint32_t requested=uint32_t(bytes)/4,n=requested<a.available?requested:a.available;
 for(uint32_t i=0;i<n;++i){out[i*2]=a.ring[a.read*2];out[i*2+1]=a.ring[a.read*2+1];a.read=(a.read+1)%RING;}
 a.available-=n;a.played+=n;if(n<requested && !a.finishing)++a.starves;
}
bool read_record(FILE* f,const alpha::StartupMovieInfo& movie,uint32_t frame,uint8_t* target,Audio& state,SDL_AudioDeviceID device){
 uint32_t samples=alpha::startup_movie_audio_frames(movie,frame);size_t bytes=movie.video_bytes+size_t(samples)*4;
 if(fread(target,1,bytes,f)!=bytes)return false;
 SDL_LockAudioDevice(device);
 if(samples>RING-state.available){SDL_UnlockAudioDevice(device);return false;}
 const uint8_t* pcm=target+movie.video_bytes;
 for(uint32_t i=0;i<samples;++i){for(uint32_t c=0;c<2;++c){uint16_t v=uint16_t(pcm[i*4+c*2])|(uint16_t(pcm[i*4+c*2+1])<<8);int32_t signed_value=v<32768?v:int32_t(v)-65536;state.ring[state.write*2+c]=int16_t(signed_value);}state.write=(state.write+1)%RING;}
 state.available+=samples;if(frame+1==movie.frames)state.finishing=true;SDL_UnlockAudioDevice(device);return true;
}
bool interrupted(struct Window* w){struct IntuiMessage* m;bool quit=false;while((m=(struct IntuiMessage*)GetMsg(w->UserPort))){ULONG c=m->Class;UWORD k=m->Code;ReplyMsg((struct Message*)m);if(c==IDCMP_CLOSEWINDOW || (c==IDCMP_VANILLAKEY && (k==13 || k==27 || k=='q' || k=='Q' || k==' ')) || (c==IDCMP_MOUSEBUTTONS && (k==SELECTDOWN || k==MENUDOWN)))quit=true;}return quit;}
}
/* Return 0 on completed playback, 5 on user skip, 20 on invalid/unavailable.
 * The player owns only its own device/window/library references. */
int alpha_startup_movie(const char* path){
 if(!path || SDL_WasInit(SDL_INIT_AUDIO))return 20;
 FILE* f=fopen(path,"rb");if(!f)return 20;
 uint8_t header[40];alpha::StartupMovieInfo movie;bool valid=false;
 if(!fseek(f,0,SEEK_END)){long bytes=ftell(f);rewind(f);valid=bytes>=40 && fread(header,1,40,f)==40 && alpha::startup_movie_header(header,40,uint64_t(bytes),movie);}
 if(!valid){fclose(f);puts("STARTUP invalid movie cache");return 20;}
 auto* record=static_cast<uint8_t*>(malloc(movie.max_record_bytes*4));auto* rgb=static_cast<uint8_t*>(malloc(size_t(movie.width)*movie.height*3));auto* state=static_cast<Audio*>(calloc(1,sizeof(Audio)));
 if(!record || !rgb || !state){free(record);free(rgb);free(state);fclose(f);return 20;}
 IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);
 struct Window* window=nullptr;SDL_AudioDeviceID device=0;int rc=20;uint32_t displayed=0,max_video_late=0;bool audio_init=false;
 if(IntuitionBase && GfxBase && CyberGfxBase)window=OpenWindowTags(nullptr,WA_Title,(ULONG)"Alpha original startup movie - Esc/Space skips",WA_InnerWidth,movie.width,WA_InnerHeight,movie.height,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS,TAG_DONE);
 if(window && SDL_InitSubSystem(SDL_INIT_AUDIO)==0){audio_init=true;SDL_AudioSpec spec{};spec.freq=22050;spec.format=AUDIO_S16SYS;spec.channels=2;spec.samples=512;spec.callback=sound;spec.userdata=state;device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);}
 if(device){
  printf("STARTUP BEGIN width=%lu height=%lu frames=%lu audio=%lu\n",(unsigned long)movie.width,(unsigned long)movie.height,(unsigned long)movie.frames,(unsigned long)movie.audio_frames);fflush(stdout);
  rc=0;uint32_t start=SDL_GetTicks();bool started=false;
  uint32_t loaded=0;
  for(;loaded<movie.frames && loaded<4;++loaded)if(!read_record(f,movie,loaded,record+loaded*movie.max_record_bytes,*state,device)){rc=20;break;}
  for(uint32_t frame=0;frame<movie.frames && !rc;++frame){
   uint8_t* slot=record+(frame%4)*movie.max_record_bytes;
   if(!alpha::startup_movie_rgb(movie,slot,movie.video_bytes,rgb,size_t(movie.width)*movie.height*3)){rc=20;break;}
   while(!rc){SDL_LockAudioDevice(device);uint32_t played=state->played;SDL_UnlockAudioDevice(device);if(played>=frame*1470)break;if(interrupted(window)){rc=5;break;}if(SDL_GetTicks()-start>movie.frames*1000/15+10000){rc=20;break;}SDL_Delay(1);}
   if(rc)break;
   WritePixelArray(rgb,0,0,movie.width*3,window->RPort,window->BorderLeft,window->BorderTop,movie.width,movie.height,RECTFMT_RGB);++displayed;
   SDL_LockAudioDevice(device);uint32_t presented=state->played;SDL_UnlockAudioDevice(device);
   if(presented>frame*1470 && presented-frame*1470>max_video_late)max_video_late=presented-frame*1470;
   if(!started){start=SDL_GetTicks();SDL_PauseAudioDevice(device,0);started=true;}
   if(frame==0){printf("STARTUP FIRST FRAME\n");fflush(stdout);}
   if(interrupted(window))rc=5;
   if(!rc && loaded<movie.frames){if(!read_record(f,movie,loaded,slot,*state,device))rc=20;else ++loaded;}
  }
  while(!rc){SDL_LockAudioDevice(device);bool finished=state->played>=movie.audio_frames;SDL_UnlockAudioDevice(device);if(finished)break;if(interrupted(window)){rc=5;break;}if(SDL_GetTicks()-start>movie.frames*1000/15+10000){rc=20;break;}SDL_Delay(1);}
  SDL_PauseAudioDevice(device,1);printf("STARTUP END rc=%d displayed=%lu audio_played=%lu starves=%lu elapsed=%lu max_video_late_samples=%lu\n",rc,(unsigned long)displayed,(unsigned long)state->played,(unsigned long)state->starves,(unsigned long)(SDL_GetTicks()-start),(unsigned long)max_video_late);fflush(stdout);
 }
 if(device)SDL_CloseAudioDevice(device);
 if(audio_init)SDL_QuitSubSystem(SDL_INIT_AUDIO);
 if(window)CloseWindow(window);
 if(CyberGfxBase)CloseLibrary(CyberGfxBase);
 if(GfxBase)CloseLibrary((struct Library*)GfxBase);
 if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);
 free(record);free(rgb);free(state);fclose(f);return rc;
}
#ifndef ALPHA_EMBEDDED_MOVIE
int main(int argc,char** argv){if(argc!=2){puts("Usage: alpha-startup-movie original.amov");return 20;}return alpha_startup_movie(argv[1]);}
#endif
