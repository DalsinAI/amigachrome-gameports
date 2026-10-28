/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_service.h"
#include "audio_music.h"
#include "audio_wav.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
namespace alpha {
namespace {
struct Clip { char name[audio_name_bytes]; int16_t* samples; uint32_t frames, rate, last_use; bool missing; };
const int clip_slots=24;
const size_t clip_budget=4u*1024u*1024u; // decoded effect bytes kept resident
const size_t max_file=8u*1024u*1024u;
struct State {
 bool open=false, owns_sdl=false, suspended=false, interlude=false;
 SDL_AudioDeviceID device=0;
 SDL_Thread* feeder=nullptr; SDL_mutex* pump_lock=nullptr; SDL_atomic_t running{};
 char directory[160]="";
 AudioSettings settings;
 AudioMixer mixer;
 MusicPump pump;
 Clip clips[clip_slots];
 uint32_t clock=0;
 int16_t* voice=nullptr;
 unsigned missing=0, failures=0;
 const char* error="closed";
};
State* state=nullptr;

void lock_device(void* ctx){auto* s=(State*)ctx;if(s->device)SDL_LockAudioDevice(s->device);}
void unlock_device(void* ctx){auto* s=(State*)ctx;if(s->device)SDL_UnlockAudioDevice(s->device);}
void callback(void* ctx,Uint8* stream,int bytes){((State*)ctx)->mixer.render((int16_t*)stream,(size_t)bytes/2);}
int feed(void* ctx){
 auto* s=(State*)ctx;
 while(SDL_AtomicGet(&s->running)){
  SDL_LockMutex(s->pump_lock);
  if(s->settings.levels.music_on)music_pump(s->pump,s->mixer,lock_device,unlock_device,s);
  SDL_UnlockMutex(s->pump_lock);
  SDL_Delay(30);
 }
 return 0;
}
bool exists(void* ctx,const char* rel){
 char path[256];if(!join_audio_path(((State*)ctx)->directory,rel,path,sizeof(path)))return false;
 FILE* f=fopen(path,"rb");if(!f)return false;fclose(f);return true;
}
uint8_t* read_file(const char* path,size_t& size,long minimum=12){
 size=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);
 if(n<minimum||n<1||(size_t)n>max_file||fseek(f,0,SEEK_SET)){fclose(f);return nullptr;}
 auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}
 if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);p=nullptr;}else size=(size_t)n;fclose(f);return p;
}
// Decodes any supported WAV into a malloc'd mono buffer.
int16_t* load_wav(const char* path,uint32_t& frames,uint32_t& rate,bool& missing){
 size_t n=0;uint8_t* raw=read_file(path,n);missing=!raw;if(!raw)return nullptr;
 WavInfo w;int16_t* pcm=nullptr;size_t count=0;
 if(parse_wav(raw,n,n,w)==WavError::ok&&(pcm=(int16_t*)malloc((size_t)w.frames*2))!=nullptr&&decode_wav_mono(raw,n,w,pcm,w.frames,count)==WavError::ok){frames=(uint32_t)count;rate=w.rate;}
 else{free(pcm);pcm=nullptr;}
 free(raw);return pcm;
}
size_t resident(){size_t b=0;for(auto& c:state->clips)if(c.samples)b+=(size_t)c.frames*2;return b;}
void evict_for(size_t bytes){
 // Least recently used first; never a buffer the mixer still reads.
 for(;;){
  if(resident()+bytes<=clip_budget)return;
  int victim=-1;
  SDL_LockAudioDevice(state->device);
  for(int i=0;i<clip_slots;++i){auto& c=state->clips[i];if(c.samples&&!state->mixer.references(c.samples)&&(victim<0||c.last_use<state->clips[victim].last_use))victim=i;}
  SDL_UnlockAudioDevice(state->device);
  if(victim<0)return;
  free(state->clips[victim].samples);memset(&state->clips[victim],0,sizeof(Clip));
 }
}
const Clip* effect(const char* name){
 auto& s=*state;
 for(auto& c:s.clips)if(c.name[0]&&!strcmp(c.name,name)){c.last_use=++s.clock;return c.missing?nullptr:&c;}
 char rel[96],path[256];uint32_t frames=0,rate=0;bool missing=true;int16_t* pcm=nullptr;
 // Original layout keeps effects in fx/; a flattened private copy is also accepted.
 const char* layouts[2]={"fx/%s","%s"};
 for(auto* layout:layouts){snprintf(rel,sizeof(rel),layout,name);if(!join_audio_path(s.directory,rel,path,sizeof(path)))continue;
  bool gone=true;pcm=load_wav(path,frames,rate,gone);if(pcm||!gone){missing=gone;break;}}
 if(missing)++s.missing;else if(!pcm)++s.failures;
 if(pcm)evict_for((size_t)frames*2);
 // An empty slot, else the least recently used entry nothing is playing.
 int slot=-1;
 for(int i=0;i<clip_slots&&slot<0;++i)if(!s.clips[i].name[0])slot=i;
 if(slot<0){
  SDL_LockAudioDevice(s.device);
  for(int i=0;i<clip_slots;++i){auto& c=s.clips[i];if(c.samples&&s.mixer.references(c.samples))continue;if(slot<0||c.last_use<s.clips[slot].last_use)slot=i;}
  SDL_UnlockAudioDevice(s.device);
 }
 if(slot<0){free(pcm);return nullptr;}
 auto& c=s.clips[slot];free(c.samples);memset(&c,0,sizeof(Clip));
 snprintf(c.name,sizeof(c.name),"%s",name);c.samples=pcm;c.frames=frames;c.rate=rate;c.missing=!pcm;c.last_use=++s.clock;
 return pcm?&c:nullptr;
}
bool start_voice(const char* rel){
 auto& s=*state;if(!s.device||!s.settings.levels.voice_on)return false;
 char path[256];if(!join_audio_path(s.directory,rel,path,sizeof(path)))return false;
 uint32_t frames=0,rate=0;bool missing=false;int16_t* pcm=load_wav(path,frames,rate,missing);
 if(!pcm){if(missing)++s.missing;else ++s.failures;s.error=missing?"voice recording missing":"voice recording invalid";return false;}
 SDL_LockAudioDevice(s.device);s.mixer.stop_voice();AudioClip clip{pcm,frames,rate};s.mixer.play_voice(clip);SDL_UnlockAudioDevice(s.device);
 free(s.voice);s.voice=pcm;return true;
}
bool open_device(){
 auto& s=*state;SDL_AudioSpec want{};want.freq=(int)AudioMixer::output_rate;want.format=AUDIO_S16SYS;want.channels=1;want.samples=1024;want.callback=callback;want.userdata=state;
 s.device=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);
 if(!s.device){s.error=SDL_GetError();return false;}
 SDL_AtomicSet(&s.running,1);s.feeder=SDL_CreateThread(feed,"alpha-music",state);
 SDL_PauseAudioDevice(s.device,0);s.error="ok";return true;
}
void close_device(){
 auto& s=*state;
 if(s.feeder){SDL_AtomicSet(&s.running,0);SDL_WaitThread(s.feeder,nullptr);s.feeder=nullptr;}
 if(s.device)SDL_CloseAudioDevice(s.device);
 s.device=0;
}
}
bool audio_open(const char* dir,const char* settings_path){
 if(state)return state->device!=0;
 state=(State*)calloc(1,sizeof(State));if(!state)return false;new(state)State();
 auto& s=*state;s.open=true;
 if(dir)snprintf(s.directory,sizeof(s.directory),"%s",dir);
 snprintf(s.pump.directory,sizeof(s.pump.directory),"%s",s.directory);
 if(settings_path){size_t n=0;uint8_t* text=read_file(settings_path,n,1);
  if(text){if(parse_audio_settings((const char*)text,n,s.settings))s.error="settings file rejected; defaults used";free(text);}}
 s.mixer.reset();s.mixer.set_levels(s.settings.levels);
 s.pump_lock=SDL_CreateMutex();
 if(!SDL_WasInit(SDL_INIT_AUDIO)){if(SDL_InitSubSystem(SDL_INIT_AUDIO)){s.error=SDL_GetError();return false;}s.owns_sdl=true;}
 return s.pump_lock&&open_device();
}
void audio_close(){
 if(!state)return;
 auto& s=*state;close_device();
 s.pump.stream.close();
 for(auto& c:s.clips)free(c.samples);
 free(s.voice);
 if(s.pump_lock)SDL_DestroyMutex(s.pump_lock);
 if(s.owns_sdl)SDL_QuitSubSystem(SDL_INIT_AUDIO);
 s.~State();free(state);state=nullptr;
}
bool audio_ready(){return state&&state->device;}
void audio_suspend(){if(!state||state->suspended)return;close_device();state->suspended=true;}
bool audio_resume(){if(!state)return false;if(!state->suspended)return state->device!=0;state->suspended=false;return open_device();}
void audio_event(AudioEvent e){
 if(!state||!state->device||!state->settings.levels.sfx_on)return;
 const char* name=audio_event_file(state->settings,e);if(!name)return;
 const Clip* c=effect(name);if(!c)return;
 AudioClip clip{c->samples,c->frames,c->rate};
 SDL_LockAudioDevice(state->device);state->mixer.play_sfx(clip);SDL_UnlockAudioDevice(state->device);
}
bool audio_world_music(const char* stem){
 if(!state)return false;
 auto& s=*state;
 SDL_LockMutex(s.pump_lock);
 s.pump.stream.close();s.pump.stopped=false;s.pump.waiting=false;s.pump.playlist.build(stem,exists,state);
 if(s.device){SDL_LockAudioDevice(s.device);s.mixer.music_stop();SDL_UnlockAudioDevice(s.device);}
 bool any=s.pump.playlist.count>0;if(!any)s.error="no prepared music tracks (music/<set>.wav)";
 SDL_UnlockMutex(s.pump_lock);return any;
}
void audio_music_stop(){
 if(!state)return;
 auto& s=*state;SDL_LockMutex(s.pump_lock);s.pump.stream.close();s.pump.playlist.count=0;
 if(s.device){SDL_LockAudioDevice(s.device);s.mixer.music_stop();SDL_UnlockAudioDevice(s.device);}SDL_UnlockMutex(s.pump_lock);
}
bool audio_faction_speech(const char* stem,bool expansion){
 char rel[64];return state&&faction_voice_path(stem,expansion,rel,sizeof(rel))&&start_voice(rel);
}
bool audio_tech_discovered(int id,bool human){
 if(!state)return false;
 audio_event(AudioEvent::tech_discovered);
 char rel[64];return human&&technology_voice_path(id,rel,sizeof(rel))&&start_voice(rel);
}
bool audio_facility_complete(int id,bool first){
 if(!state)return false;
 audio_event(AudioEvent::facility_complete);
 char rel[64];return first&&facility_voice_path(id,rel,sizeof(rel))&&start_voice(rel);
}
void audio_interlude(int){
 if(!state||!state->device)return;
 state->interlude=true;
 SDL_LockAudioDevice(state->device);state->mixer.fade_voice(2000);
 MixerLevels l=state->settings.levels;l.music=(uint8_t)(l.music*l.music_duck/100);state->mixer.set_levels(l);
 SDL_UnlockAudioDevice(state->device);
}
void audio_interlude_closed(){
 if(!state||!state->device)return;
 state->interlude=false;
 SDL_LockAudioDevice(state->device);state->mixer.set_levels(state->settings.levels);SDL_UnlockAudioDevice(state->device);
}
void audio_popup_closed(){
 if(!state||!state->device||!state->settings.voice_stops_on_close)return;
 SDL_LockAudioDevice(state->device);state->mixer.fade_voice(2000);SDL_UnlockAudioDevice(state->device);
}
void audio_stop_voice(){if(!state||!state->device)return;SDL_LockAudioDevice(state->device);state->mixer.stop_voice();SDL_UnlockAudioDevice(state->device);}
static const AudioSettings default_settings;
const AudioSettings& audio_settings(){return state?state->settings:default_settings;}
void audio_set_settings(const AudioSettings& settings){
 if(!state)return;
 auto& s=*state;
 SDL_LockMutex(s.pump_lock);s.settings=settings;SDL_UnlockMutex(s.pump_lock);
 if(s.device){SDL_LockAudioDevice(s.device);s.mixer.set_levels(settings.levels);if(!settings.levels.voice_on)s.mixer.stop_voice();if(!settings.levels.sfx_on)s.mixer.stop_sfx();SDL_UnlockAudioDevice(s.device);}
 if(s.interlude)audio_interlude(0);
}
bool audio_save_settings(const char* path){
 if(!path)return false;
 char text[4096];size_t n=format_audio_settings(audio_settings(),text,sizeof(text));if(!n)return false;
 FILE* f=fopen(path,"wb");if(!f)return false;bool ok=fwrite(text,1,n,f)==n;return fclose(f)==0&&ok;
}
AudioServiceStatus audio_status(){
 AudioServiceStatus r{};if(!state){r.last_error="closed";return r;}auto& s=*state;
 r.ready=s.device!=0;r.last_error=s.error;r.missing_files=s.missing;r.decode_failures=s.failures;r.tracks_started=s.pump.tracks_started;
 for(auto& c:s.clips)if(c.samples)++r.cached_clips;
 if(s.device)SDL_LockAudioDevice(s.device);
 r.voice_playing=s.mixer.voice_active();r.music_playing=s.pump.stream.active()||s.mixer.music_fill()>0;r.music_underruns=s.mixer.stats().music_underruns;
 if(s.device)SDL_UnlockAudioDevice(s.device);
 return r;
}
}
