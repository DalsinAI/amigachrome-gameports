/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_music.h"
#include <string.h>
#ifdef __amigaos__
#include <proto/dos.h>
#include <dos/dos.h>
#endif
namespace alpha {
bool join_audio_path(const char* dir,const char* rel,char* out,size_t cap){
 if(!rel||!out||!cap)return false;
 size_t dn=dir?strlen(dir):0;
 bool sep=dn&&dir[dn-1]!='/'&&dir[dn-1]!=':';
 int n=snprintf(out,cap,"%s%s%s",dn?dir:"",sep?"/":"",rel);return n>=0&&(size_t)n<cap;
}
#ifdef __amigaos__
void* MusicStreamer::file_open(const char* path,long& size){
 BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);if(!f)return nullptr;
 if(Seek(f,0,OFFSET_END)<0){Close(f);return nullptr;}size=Seek(f,0,OFFSET_BEGINNING);
 if(size<0){Close(f);return nullptr;}return (void*)f;
}
size_t MusicStreamer::file_read(void* f,void* out,size_t n){LONG got=Read((BPTR)f,out,(LONG)n);return got>0?(size_t)got:0;}
bool MusicStreamer::file_seek(void* f,long offset){return Seek((BPTR)f,offset,OFFSET_BEGINNING)>=0;}
void MusicStreamer::file_close(void* f){if(f)Close((BPTR)f);}
#else
void* MusicStreamer::file_open(const char* path,long& size){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)||(size=ftell(f))<0||fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}return f;
}
size_t MusicStreamer::file_read(void* f,void* out,size_t n){return fread(out,1,n,(FILE*)f);}
bool MusicStreamer::file_seek(void* f,long offset){return fseek((FILE*)f,offset,SEEK_SET)==0;}
void MusicStreamer::file_close(void* f){if(f)fclose((FILE*)f);}
#endif
WavError MusicStreamer::open(const char* path){
 close();long size=0;void* f=file_open(path,size);if(!f)return WavError::truncated;
 if(size<12){file_close(f);return WavError::truncated;}
 size_t prefix=file_read(f,raw_,sizeof(raw_));WavInfo w;
 WavError e=parse_wav(raw_,prefix,(size_t)size,w);
 if(e==WavError::ok&&!wav_frame_bytes(w))e=WavError::unsupported;
 if(e==WavError::ok&&!file_seek(f,(long)w.data_offset))e=WavError::truncated;
 if(e!=WavError::ok){file_close(f);return e;}
 file_=f;info_=w;left_=w.frames;return WavError::ok;
}
void MusicStreamer::close(){file_close(file_);file_=nullptr;left_=0;}
size_t MusicStreamer::read(int16_t* out,size_t max){
 if(!file_||!left_)return 0;
 size_t fb=wav_frame_bytes(info_);
 size_t want=max<left_?max:left_;if(want>sizeof(raw_)/fb)want=sizeof(raw_)/fb;
 size_t got=file_read(file_,raw_,want*fb)/fb;
 if(got<want)left_=got; // short file: finish after what was read
 convert_pcm_frames(raw_,got,info_,out);left_-=got;return got;
}
size_t music_pump(MusicPump& p,AudioMixer& m,AudioLockFn lock,AudioLockFn unlock,void* ctx){
 if(p.stopped||!p.playlist.count)return 0;
 if(!p.stream.active()||p.stream.at_end()){
  p.stream.close();
  for(size_t tries=0;tries<p.playlist.count&&!p.stream.active();++tries){
   char path[256];const char* t=p.playlist.next();
   if(join_audio_path(p.directory,t,path,sizeof(path))&&p.stream.open(path)==WavError::ok)++p.tracks_started;else ++p.failures;
  }
  lock(ctx);
  if(!p.stream.active()){m.music_end_of_stream();unlock(ctx);p.stopped=true;return 0;}
  // Same source rate: continue the ring gaplessly. Otherwise drain, then restart.
  if(m.music_live()&&!m.music_finished()&&AudioMixer::step_for(p.stream.rate())==m.music_rate_step())m.music_continue();
  else{m.music_end_of_stream();p.waiting=true;}
  unlock(ctx);
 }
 if(p.waiting){
  lock(ctx);bool drained=!m.music_live()||m.music_finished();if(drained)m.music_start(p.stream.rate());unlock(ctx);
  if(!drained)return 0;
  p.waiting=false;
 }
 size_t pushed=0;
 for(int round=0;round<8&&!p.stream.at_end();++round){
  lock(ctx);size_t space=m.music_space();unlock(ctx);
  if(space<512)break;
  size_t want=space<2048?space:2048;size_t n=p.stream.read(p.scratch,want);
  // Only this producer writes the ring, so `space` cannot shrink meanwhile.
  lock(ctx);m.music_push(p.scratch,n);unlock(ctx);pushed+=n;
  if(!n)break;
 }
 return pushed;
}
}
