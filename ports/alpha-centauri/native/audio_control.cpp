/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP10 audio controls: synthetic WAV fixtures only, no original media.
 * Usage: audio-control SCRATCH_DIR [ORACLE_DIR]
 *        audio-control --bench KIND FILE   (KIND: decode | mix)
 */
#include "audio_events.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "audio_wav.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static unsigned checks,failures;
static bool check(bool good,const char* what){++checks;if(!good){++failures;printf("FAIL %u %s\n",checks,what);}return good;}
#define CHECK(x) check((x),#x)
namespace {
struct Buf { uint8_t b[65536]; size_t n=0;
 void raw(const void* p,size_t k){memcpy(b+n,p,k);n+=k;}
 void u16(unsigned v){b[n++]=(uint8_t)v;b[n++]=(uint8_t)(v>>8);}
 void u32(uint32_t v){u16(v&0xFFFF);u16(v>>16);}
 void tag(const char* t){raw(t,4);}
};
// fmt chunk for PCM; optional junk chunks before/after it.
size_t make_pcm(Buf& w,unsigned channels,unsigned rate,unsigned bits,const void* data,size_t bytes,bool junk=false,long riff_adjust=0){
 w.n=0;w.tag("RIFF");w.u32(0);w.tag("WAVE");
 if(junk){w.tag("LIST");w.u32(3);w.raw("abc",3);w.b[w.n++]=0;}
 w.tag("fmt ");w.u32(16);w.u16(1);w.u16(channels);w.u32(rate);w.u32(rate*channels*bits/8);w.u16(channels*bits/8);w.u16(bits);
 w.tag("data");w.u32((uint32_t)bytes);w.raw(data,bytes);if(bytes&1)w.b[w.n++]=0;
 if(junk){w.tag("cue ");w.u32(2);w.u16(7);}
 uint32_t riff=(uint32_t)(w.n-8+riff_adjust);w.b[4]=(uint8_t)riff;w.b[5]=(uint8_t)(riff>>8);w.b[6]=(uint8_t)(riff>>16);w.b[7]=(uint8_t)(riff>>24);
 return w.n;
}
uint8_t* slurp(const char* path,size_t& n){
 n=0;FILE* f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long s=ftell(f);fseek(f,0,SEEK_SET);
 auto* p=(uint8_t*)malloc(s>0?(size_t)s:1);if(p&&fread(p,1,(size_t)s,f)==(size_t)s)n=(size_t)s;else{free(p);p=nullptr;}fclose(f);return p;
}
bool spit(const char* path,const void* p,size_t n){FILE* f=fopen(path,"wb");if(!f)return false;bool ok=fwrite(p,1,n,f)==n;return fclose(f)==0&&ok;}
void wav_tests(){
 int16_t pcm[4]={0,32767,-32768,-1};uint8_t le[8];for(int i=0;i<4;++i){le[2*i]=(uint8_t)pcm[i];le[2*i+1]=(uint8_t)((uint16_t)pcm[i]>>8);}
 Buf w;make_pcm(w,1,22050,16,le,8);WavInfo info;int16_t out[16];size_t n=99;
 CHECK(parse_wav(w.b,w.n,w.n,info)==WavError::ok&&info.codec==WavCodec::pcm_s16&&info.frames==4&&info.rate==22050&&!info.clamped);
 CHECK(decode_wav_mono(w.b,w.n,info,out,16,n)==WavError::ok&&n==4&&out[0]==0&&out[1]==32767&&out[2]==-32768&&out[3]==-1);
 // Capacity failure is transactional.
 n=99;out[0]=123;CHECK(decode_wav_mono(w.b,w.n,info,out,3,n)==WavError::capacity&&n==99&&out[0]==123);
 // Every truncated prefix is rejected or clamped without reading past the end.
 unsigned rejected=0,clamped=0;
 for(size_t k=0;k<w.n;++k){auto* copy=(uint8_t*)malloc(k?k:1);memcpy(copy,w.b,k);WavInfo t;WavError e=parse_wav(copy,k,k,t);
  if(e!=WavError::ok)++rejected;else{++clamped;size_t m=0;int16_t o[8];if(decode_wav_mono(copy,k,t,o,8,m)!=WavError::ok||m!=t.frames)++failures;}free(copy);}
 CHECK(rejected>=44&&rejected+clamped==w.n);
 // Unknown chunks, odd padding and a stale (too large) RIFF size are tolerated.
 uint8_t three[3]={128,255,0};make_pcm(w,1,11025,8,three,3,true,1000);
 CHECK(parse_wav(w.b,w.n,w.n,info)==WavError::ok&&info.codec==WavCodec::pcm_u8&&info.frames==3&&info.rate==11025);
 CHECK(decode_wav_mono(w.b,w.n,info,out,16,n)==WavError::ok&&n==3&&out[0]==0&&out[1]==32512&&out[2]==-32768);
 // Stereo is averaged to mono (u8 and s16).
 uint8_t st8[4]={255,1,128,128};make_pcm(w,2,22050,8,st8,4);
 CHECK(parse_wav(w.b,w.n,w.n,info)==WavError::ok&&info.frames==2&&decode_wav_mono(w.b,w.n,info,out,16,n)==WavError::ok&&out[0]==0&&out[1]==0);
 uint8_t st16[8]={0xFF,0x7F,0x01,0x00, 0x00,0x80,0xFF,0xFF};make_pcm(w,2,22050,16,st16,8);
 CHECK(parse_wav(w.b,w.n,w.n,info)==WavError::ok&&decode_wav_mono(w.b,w.n,info,out,16,n)==WavError::ok&&n==2&&out[0]==16384&&out[1]==-16385);
 // Data chunk longer than the file: clamped to whole frames and flagged.
 make_pcm(w,1,22050,16,le,8);w.b[40]=200;CHECK(parse_wav(w.b,w.n,w.n,info)==WavError::ok&&info.clamped&&info.frames==4);
 // Rejections.
 make_pcm(w,1,22050,16,le,8);uint8_t bad[128];
 struct { size_t at; uint8_t v; WavError e; } cases[]={{0,'X',WavError::not_riff},{8,'X',WavError::not_riff},{20,3,WavError::unsupported},{22,3,WavError::bad_format},{34,12,WavError::unsupported},{32,3,WavError::bad_format},{26,0x10,WavError::bad_format}};
 for(auto& c:cases){memcpy(bad,w.b,w.n);bad[c.at]=c.v;WavInfo t;WavError e=parse_wav(bad,w.n,w.n,t);if(c.e==WavError::ok)check(e!=WavError::ok,"rate below range");else check(e==c.e,"rejection case");}
 // Duplicate fmt chunk, data before fmt, missing data.
 Buf d;d.tag("RIFF");d.u32(0);d.tag("WAVE");for(int k=0;k<2;++k){d.tag("fmt ");d.u32(16);d.u16(1);d.u16(1);d.u32(22050);d.u32(44100);d.u16(2);d.u16(16);}d.tag("data");d.u32(2);d.u16(5);
 CHECK(parse_wav(d.b,d.n,d.n,info)==WavError::bad_chunk);
 Buf e;e.tag("RIFF");e.u32(0);e.tag("WAVE");e.tag("data");e.u32(2);e.u16(5);CHECK(parse_wav(e.b,e.n,e.n,info)==WavError::bad_chunk);
 Buf f;f.tag("RIFF");f.u32(0);f.tag("WAVE");f.tag("fmt ");f.u32(16);f.u16(1);f.u16(1);f.u32(22050);f.u32(44100);f.u16(2);f.u16(16);CHECK(parse_wav(f.b,f.n,f.n,info)==WavError::no_data);
 // Streaming prefix: header-only buffer of a larger file parses.
 make_pcm(w,1,22050,16,le,8);CHECK(parse_wav(w.b,44,w.n,info)==WavError::ok&&info.data_offset==44&&info.frames==4);
 CHECK(parse_wav(w.b,30,w.n,info)==WavError::truncated);
}
// ffmpeg-produced ADPCM/PCM fixtures with ffmpeg-decoded references (exact match).
void oracle_tests(const char* dir){
 const char* names[]={"ima_mono","ima_stereo","ms_mono","ms_stereo","u8_stereo","s16_44k"};
 for(auto* name:names){
  char a[512],b[512];snprintf(a,sizeof(a),"%s/%s.wav",dir,name);snprintf(b,sizeof(b),"%s/%s.s16",dir,name);
  size_t wn=0,rn=0;uint8_t* wav=slurp(a,wn);uint8_t* ref=slurp(b,rn);
  if(!check(wav&&ref,name)){free(wav);free(ref);continue;}
  WavInfo info;bool ok=parse_wav(wav,wn,wn,info)==WavError::ok;size_t frames=info.frames;
  auto* out=(int16_t*)malloc(frames*2+2);size_t n=0;ok=ok&&decode_wav_mono(wav,wn,info,out,frames,n)==WavError::ok;
  // The reference is ffmpeg's native decode (mono, or interleaved stereo averaged here).
  size_t ref_frames=rn/2/info.channels;size_t match=0;
  if(ok)for(size_t i=0;i<n&&i<ref_frames;++i){
   int32_t r;const uint8_t* p=ref+2*info.channels*i;int16_t l=(int16_t)(p[0]|p[1]<<8);
   if(info.channels==2){int16_t rr=(int16_t)(p[2]|p[3]<<8);r=info.codec==WavCodec::pcm_u8?l/2+rr/2:((int32_t)l+rr)>>1;}else r=l;
   // u8 stereo: (a+b-256)*128 equals ffmpeg's ((a-128)<<8 + (b-128)<<8)/2 exactly.
   if(info.codec==WavCodec::pcm_u8&&info.channels==2){int32_t x=(int16_t)(p[0]|p[1]<<8),y=(int16_t)(p[2]|p[3]<<8);r=(x+y)/2;}
   if(out[i]==r)++match;}
  char what[128];snprintf(what,sizeof(what),"oracle %s frames=%lu ref=%lu match=%lu",name,(unsigned long)n,(unsigned long)ref_frames,(unsigned long)match);
  check(ok&&n==ref_frames&&match==n,what);if(ok)printf("ORACLE %s codec=%d ch=%u rate=%lu frames=%lu exact\n",name,(int)info.codec,info.channels,(unsigned long)info.rate,(unsigned long)n);
  free(out);free(wav);free(ref);
 }
}
void mixer_tests(){
 static AudioMixer m;m.reset();int16_t out[1024];
 m.render(out,64);bool silent=true;for(int i=0;i<64;++i)silent&=out[i]==0;CHECK(silent);
 int16_t ramp[100];for(int i=0;i<100;++i)ramp[i]=(int16_t)(i*100-5000);
 AudioClip clip{ramp,100,22050};
 MixerLevels l;l.music=100;m.set_levels(l);
 CHECK(m.play_sfx(clip)==0);m.render(out,120);bool same=true;for(int i=0;i<100;++i)same&=out[i]==ramp[i];for(int i=100;i<120;++i)same&=out[i]==0;CHECK(same);CHECK(!m.sfx_active());
 // 50% clip gain, 50% bus, 50% master -> 1/8 (Q12 truncation toward -inf).
 l.sfx=50;l.master=50;m.set_levels(l);m.play_sfx(clip,50);m.render(out,100);same=true;for(int i=0;i<100;++i)same&=out[i]==(int16_t)((ramp[i]*((((4096*50/100)*50/100)*2048)>>12))>>12);CHECK(same);
 l=MixerLevels{};l.sfx_on=false;m.set_levels(l);m.play_sfx(clip);m.render(out,100);silent=true;for(int i=0;i<100;++i)silent&=out[i]==0;CHECK(silent);
 // Clipping saturates and is counted.
 l=MixerLevels{};m.set_levels(l);m.stop_sfx();int16_t loud[4]={30000,30000,-30000,-30000};AudioClip lc{loud,4,22050};unsigned before=m.stats().clipped;
 m.play_sfx(lc);m.play_sfx(lc);m.render(out,4);CHECK(out[0]==32767&&out[2]==-32768&&m.stats().clipped==before+4);
 // Slot stealing: the oldest effect is replaced.
 int16_t one[2000];for(auto& v:one)v=1;AudioClip oc{one,2000,22050};for(int i=0;i<AudioMixer::sfx_slots;++i)m.play_sfx(oc);
 unsigned stolen=m.stats().sfx_stolen;CHECK(m.play_sfx(oc)==0&&m.stats().sfx_stolen==stolen+1);m.stop_sfx();
 // 11025 Hz source: every other output sample is the linear midpoint.
 int16_t half[3]={0,1000,-1000};AudioClip hc{half,3,11025};m.play_sfx(hc);m.render(out,8);
 CHECK(out[0]==0&&out[1]==500&&out[2]==1000&&out[3]==0&&out[4]==-1000&&out[5]==-500&&out[6]==0);
 // Load-time resampling matches the live mixer path exactly.
 int16_t up[8];CHECK(resampled_frames(3,11025,22050)==6&&resample_linear(half,3,11025,up,8,22050)==6&&up[0]==0&&up[1]==500&&up[2]==1000&&up[3]==0&&up[4]==-1000&&up[5]==-1000);
 CHECK(resample_linear(half,3,11025,up,5,22050)==0&&resampled_frames(4,44100,22050)==2&&resample_linear(ramp,4,44100,up,8,22050)==2&&up[0]==ramp[0]&&up[1]==ramp[2]);
 // References guard.
 m.play_sfx(oc);CHECK(m.references(one)&&!m.references(half));m.stop_sfx();CHECK(!m.references(one));
 // Voice: plays once; fade stops it after the fade length.
 int16_t vo[22050];for(auto& v:vo)v=1000;AudioClip vc{vo,22050,22050};m.play_voice(vc);CHECK(m.voice_active());
 m.render(out,100);CHECK(out[0]==1000&&out[99]==1000);m.fade_voice(10);m.render(out,300);CHECK(!m.voice_active()&&out[0]<1000&&out[0]>900&&out[299]==0);
 l.voice_on=false;m.set_levels(l);m.play_voice(vc);m.render(out,10);CHECK(out[0]==0);m.stop_voice();l.voice_on=true;
 // Music: ring push/consume, ramp, ducking under voice, mute pauses, underrun.
 m.reset();l=MixerLevels{};l.music=100;l.music_duck=50;m.set_levels(l);m.music_start(22050);
 static int16_t mus[20000];for(auto& v:mus)v=4000;CHECK(m.music_push(mus,20000)==20000&&m.music_fill()==20000);
 m.render(out,1024);CHECK(out[0]<100&&out[1023]==4000&&m.music_fill()==20000-1024); // ramped in from zero
 m.play_voice(AudioClip{vo,2000,22050});m.render(out,1024);CHECK(out[1023]==1000+2000); // music ducked to 50%
 m.stop_voice();m.render(out,1024);CHECK(out[1023]==4000);
 l.music_on=false;m.set_levels(l);m.render(out,1024);size_t fill=m.music_fill();m.render(out,1024);CHECK(out[1023]==0&&m.music_fill()==fill); // paused
 l.music_on=true;m.set_levels(l);size_t room=m.music_space();CHECK(room==AudioMixer::music_ring-1-fill);
 unsigned under=m.stats().music_underruns;m.music_stop();m.music_start(22050);m.render(out,16);CHECK(m.stats().music_underruns==under+1);
 m.music_push(mus,3);m.music_end_of_stream();m.render(out,16);CHECK(m.music_finished()&&m.stats().music_underruns==under+1);
 // 44100 Hz music consumes two source samples per output sample.
 m.music_start(44100);m.music_push(mus,1000);m.render(out,100);CHECK(m.music_fill()==800);
}
bool fake_exists(void* ctx,const char* path){for(const char** p=(const char**)ctx;*p;++p)if(!strcmp(*p,path))return true;return false;}
void settings_tests(){
 AudioSettings s;CHECK(audio_event_file(s,AudioEvent::ui_click)&&!strcmp(audio_event_file(s,AudioEvent::ui_click),"ok.wav"));
 CHECK(!audio_event_file(s,AudioEvent::unit_move)&&!strcmp(audio_event_file(s,AudioEvent::psi_destroyed),"psi blow.wav"));
 const char* text="# comment\r\nsound_effects = off\nmusic=on\nvoices=no\nmaster_volume=80\neffects_volume=0\nmusic_volume=55\nvoice_volume=100\nmusic_under_voice=30\nmap.unit_move = rover move.wav\nmap.ui_click=none\n\n  ; other comment\nvoice_stops_on_close=off\n";
 CHECK(parse_audio_settings(text,strlen(text),s)==0);
 CHECK(!s.levels.sfx_on&&s.levels.music_on&&!s.levels.voice_on&&s.levels.master==80&&s.levels.sfx==0&&s.levels.music==55&&s.levels.music_duck==30&&!s.voice_stops_on_close);
 CHECK(!strcmp(audio_event_file(s,AudioEvent::unit_move),"rover move.wav")&&!audio_event_file(s,AudioEvent::ui_click));
 char buf[2048];size_t n=format_audio_settings(s,buf,sizeof(buf));AudioSettings t;CHECK(n>0&&parse_audio_settings(buf,n,t)==0);
 CHECK(!memcmp(&t.levels,&s.levels,sizeof(MixerLevels))&&!strcmp(audio_event_file(t,AudioEvent::unit_move),"rover move.wav")&&!audio_event_file(t,AudioEvent::ui_click));
 CHECK(format_audio_settings(s,buf,40)==0);
 const char* bad[]={"music=maybe","master_volume=101","master_volume=-1","nonsense","volume=50","map.unit_move=../x.wav","map.unit_move=fx/x.wav","map.unit_move=x.mp3","map.nothing=x.wav","map.unit_move=C:x.wav"};
 for(auto* b:bad){AudioSettings u=s;check(parse_audio_settings(b,strlen(b),u)==1&&!memcmp(&u.levels,&s.levels,sizeof(MixerLevels)),b);}
 const char* second="music=on\nmusic=bad\n";CHECK(parse_audio_settings(second,strlen(second),t)==2);
 const char* reset="map.unit_move=default\n";CHECK(parse_audio_settings(reset,strlen(reset),s)==0&&!audio_event_file(s,AudioEvent::unit_move));
 AudioEvent e;CHECK(audio_event_from_key("TURN_COMPLETE",13,e)&&e==AudioEvent::turn_complete&&!audio_event_from_key("turn",4,e));
 apply_original_preferences(0x400,0x20000,s);CHECK(s.levels.sfx_on&&!s.levels.music_on&&!s.levels.voice_on&&s.voice_stops_on_close);
 apply_original_preferences(0x800,0x4000,s);CHECK(!s.levels.sfx_on&&s.levels.music_on&&s.levels.voice_on&&!s.voice_stops_on_close);
 char p[64];
 CHECK(facility_voice_path(1,p,64)&&!strcmp(p,"voices/fac1.wav"));CHECK(facility_voice_path(33,p,64)&&!strcmp(p,"voices/fac33.wav"));
 CHECK(facility_voice_path(34,p,64)&&!strcmp(p,"voices/fac340.wav"));CHECK(facility_voice_path(41,p,64)&&!strcmp(p,"voices/fac410.wav"));
 CHECK(facility_voice_path(42,p,64)&&!strcmp(p,"voices/fac42.wav"));CHECK(facility_voice_path(64,p,64)&&!strcmp(p,"voices/fac64.wav"));
 CHECK(facility_voice_path(65,p,64)&&!strcmp(p,"voices/fac34.wav"));CHECK(facility_voice_path(69,p,64)&&!strcmp(p,"voices/fac38.wav"));
 strcpy(p,"keep");CHECK(!facility_voice_path(0,p,64)&&!facility_voice_path(70,p,64)&&!facility_voice_path(-3,p,64)&&!strcmp(p,"keep"));
 CHECK(technology_voice_path(0,p,64)&&!strcmp(p,"voices/tech0.wav")&&technology_voice_path(88,p,64)&&!strcmp(p,"voices/tech88.wav"));
 CHECK(!technology_voice_path(87,p,64)&&!technology_voice_path(89,p,64)&&!technology_voice_path(-1,p,64));
 CHECK(faction_voice_path("GAIANS",false,p,64)&&!strcmp(p,"voices/gaians.wav")&&!faction_voice_path("cyborg",false,p,64)&&faction_voice_path("cyborg",true,p,64)&&!strcmp(p,"voices/cyborg.wav"));
 CHECK(!faction_voice_path("../gaians",true,p,64)&&!faction_voice_path("gaians",true,p,8));
 CHECK(!strcmp(faction_music_set("univ"),"uset2.amb")&&!strcmp(faction_music_set("Spartans"),"sset1.amb")&&!strcmp(faction_music_set("cyborg"),"aset1.amb")&&!strcmp(faction_music_set(nullptr),"aset1.amb"));
 static MusicPlaylist pl;
 const char* files1[]={"music/gset1.wav","music/gset1-2.wav","music/aset1.wav",nullptr};
 CHECK(pl.build("peace",fake_exists,files1)==2&&!strcmp(pl.next(),"music/gset1.wav")&&!strcmp(pl.next(),"music/gset1-2.wav")&&!strcmp(pl.next(),"music/gset1.wav"));
 CHECK(pl.build("hive",fake_exists,files1)==1&&!strcmp(pl.next(),"music/aset1.wav"));
 const char* files2[]={"music/bset1-1.wav",nullptr};CHECK(pl.build("univ",fake_exists,files2)==1&&!strcmp(pl.next(),"music/bset1-1.wav"));
 const char* none[]={nullptr};CHECK(pl.build("univ",fake_exists,none)==0&&!pl.next());
 for(size_t i=0;i<audio_event_count;++i){auto& info=audio_event_info((AudioEvent)i);AudioEvent back;check(info.key&&info.source&&audio_event_from_key(info.key,strlen(info.key),back)&&back==(AudioEvent)i,"catalog key");}
}
void nolock(void*){}
void pump_tests(const char* dir){
 // Two synthetic tracks; the pump must play them back-to-back, then cycle.
 static int16_t a[3000],b[1000];for(int i=0;i<3000;++i)a[i]=(int16_t)(i%200-100);for(int i=0;i<1000;++i)b[i]=(int16_t)(700+i%7);
 uint8_t raw[6000];Buf w;
 auto write=[&](const char* rel,const int16_t* s,size_t n,unsigned rate){for(size_t i=0;i<n;++i){raw[2*i]=(uint8_t)s[i];raw[2*i+1]=(uint8_t)((uint16_t)s[i]>>8);}make_pcm(w,1,rate,16,raw,2*n,true);char p[512];join_audio_path(dir,rel,p,sizeof(p));return spit(p,w.b,w.n);};
 char md[512];join_audio_path(dir,"music",md,sizeof(md));
 char cmd[600];snprintf(cmd,sizeof(cmd),"%s/marker",md);FILE* probe=fopen(cmd,"wb");if(probe)fclose(probe);
 if(!check(probe!=nullptr,"scratch music directory exists"))return;
 CHECK(write("music/sset1.wav",a,3000,22050)&&write("music/sset1-1.wav",b,1000,22050));
 static MusicPump p;p=MusicPump{};snprintf(p.directory,sizeof(p.directory),"%s",dir);
 struct Ctx{const char* dir;} ctx{dir};
 auto exists=[](void* c,const char* rel){char path[512];join_audio_path(((Ctx*)c)->dir,rel,path,sizeof(path));FILE* f=fopen(path,"rb");if(f)fclose(f);return f!=nullptr;};
 CHECK(p.playlist.build("spartans",exists,&ctx)==2);
 static AudioMixer m;m.reset();MixerLevels l;l.music=100;l.sfx_on=false;m.set_levels(l);
 static int16_t out[12000];size_t got=0;
 // Let the music ramp settle first so outputs equal source samples.
 for(int step=0;step<200&&got<sizeof(out)/2;++step){music_pump(p,m,nolock,nolock,nullptr);size_t k=256;if(got+k>sizeof(out)/2)k=sizeof(out)/2-got;m.render(out+got,k);got+=k;}
 // Find the end of track A then expect track B exactly (after the ramp, gain is unity).
 bool a_ok=true,b_ok=true;for(int i=600;i<3000;++i)a_ok&=out[i]==a[i];for(int i=0;i<1000;++i)b_ok&=out[3000+i]==b[i];
 if(!CHECK(a_ok&&b_ok&&p.tracks_started>=3&&m.stats().music_underruns==0)){printf("tracks=%u under=%u fail=%u\n",p.tracks_started,m.stats().music_underruns,p.failures);for(int i=0;i<4000;++i)if(i<3000?out[i]!=a[i]&&i>=600:out[i]!=b[i-3000]){printf("first mismatch %d got %d\n",i,out[i]);break;}}
 // A track at another rate waits for the ring to drain, then restarts it.
 {
  static int16_t c[500];for(auto& v:c)v=-900;
  CHECK(write("music/bset1.wav",b,1000,22050)&&write("music/bset1-1.wav",c,500,11025));
  static MusicPump r;r=MusicPump{};snprintf(r.directory,sizeof(r.directory),"%s",dir);
  CHECK(r.playlist.build("believe",exists,&ctx)==2);
  m.reset();m.set_levels(l);got=0;
  for(int step=0;step<40;++step){music_pump(r,m,nolock,nolock,nullptr);m.render(out+got,128);got+=128;}
  bool first=true;for(int i=600;i<1000;++i)first&=out[i]==b[i];
  // 500 frames at 11025 Hz become about 1000 output frames of -900.
  size_t start=1000;while(start<got&&out[start]!=-900)++start;
  size_t run=0;while(start+run<got&&out[start+run]==-900)++run;
  CHECK(first&&start<got&&run>=990&&run<=1000&&r.tracks_started>=2&&m.stats().music_underruns==0);
 }
 // ADPCM music and missing files are refused; a playlist of only bad files stops.
 MusicStreamer st;char path[512];join_audio_path(dir,"music/none.wav",path,sizeof(path));CHECK(st.open(path)!=WavError::ok&&!st.active());
 {
  Buf ad;ad.tag("RIFF");ad.u32(4+28+8+36);ad.tag("WAVE");ad.tag("fmt ");ad.u32(20);ad.u16(0x11);ad.u16(1);ad.u32(22050);ad.u32(11100);ad.u16(36);ad.u16(4);ad.u16(2);ad.u16(65);
  ad.tag("data");ad.u32(36);for(int i=0;i<36;++i)ad.b[ad.n++]=0;
  join_audio_path(dir,"music/adpcm.wav",path,sizeof(path));WavInfo ai;
  CHECK(spit(path,ad.b,ad.n)&&parse_wav(ad.b,ad.n,ad.n,ai)==WavError::ok&&ai.frames==65&&st.open(path)==WavError::unsupported&&!st.active());
 }
 static MusicPump q;q=MusicPump{};snprintf(q.directory,sizeof(q.directory),"%s",dir);q.playlist.count=1;snprintf(q.playlist.track[0],audio_name_bytes,"music/none.wav");
 m.music_stop();size_t qp=music_pump(q,m,nolock,nolock,nullptr);if(!CHECK(qp==0&&q.stopped&&q.failures==1))printf("qp=%lu stopped=%d failures=%u fill=%lu\n",(unsigned long)qp,q.stopped,q.failures,(unsigned long)m.music_fill());
 char s1[256];CHECK(join_audio_path("DH0:Alpha","fx/ok.wav",s1,sizeof(s1))&&!strcmp(s1,"DH0:Alpha/fx/ok.wav"));
 CHECK(join_audio_path("Work:","fx/ok.wav",s1,sizeof(s1))&&!strcmp(s1,"Work:fx/ok.wav")&&join_audio_path("","x",s1,sizeof(s1))&&!strcmp(s1,"x")&&!join_audio_path("abc","def",s1,4));
}
int bench(const char* kind,const char* file){
 if(!strcmp(kind,"mix")){
  // Typical second: two effects (pre-resampled to 22050 Hz at load, unity
  // gain), a voice line and music at the default 70%.
  static AudioMixer m;m.reset();static int16_t fx[22050],vo[22050],mus[22050],out[22050];
  for(int i=0;i<22050;++i){fx[i]=(int16_t)(i*37);vo[i]=(int16_t)(i*11);mus[i]=(int16_t)(i*5);}
  MixerLevels l;l.music_duck=100;m.set_levels(l);
  m.music_start(22050);m.music_push(mus,22050);for(int k=0;k<2;++k)m.play_sfx(AudioClip{fx,22050,22050});m.play_voice(AudioClip{vo,22050,22050});
  unsigned sum=0;for(int i=0;i<22050;i+=1024){size_t n=22050-i<1024?22050-i:1024;m.render(out+i,n);}for(auto v:out)sum=sum*31+(uint16_t)v;
  printf("BENCH mix frames=22050 sum=%08x\n",sum);return 0;
 }
 if(!strcmp(kind,"mix-worst")){
  // Worst case: three 11025 Hz effects resampled live, a voice and ducked music.
  static AudioMixer m;m.reset();static int16_t fx[22050],vo[22050],mus[22050],out[22050];
  for(int i=0;i<22050;++i){fx[i]=(int16_t)(i*37);vo[i]=(int16_t)(i*11);mus[i]=(int16_t)(i*5);}
  m.music_start(22050);m.music_push(mus,22050);for(int k=0;k<3;++k)m.play_sfx(AudioClip{fx,22050,11025});m.play_voice(AudioClip{vo,22050,22050});
  unsigned sum=0;for(int i=0;i<22050;i+=1024){size_t n=22050-i<1024?22050-i:1024;m.render(out+i,n);}for(auto v:out)sum=sum*31+(uint16_t)v;
  printf("BENCH mix frames=22050 sum=%08x\n",sum);return 0;
 }
 size_t n=0;uint8_t* wav=slurp(file,n);WavInfo info;if(!wav||parse_wav(wav,n,n,info)!=WavError::ok)return 20;
 auto* out=(int16_t*)malloc(info.frames*2);size_t count=0;if(!out||decode_wav_mono(wav,n,info,out,info.frames,count)!=WavError::ok)return 20;
 unsigned sum=0;for(size_t i=0;i<count;++i)sum=sum*31+(uint16_t)out[i];printf("BENCH decode codec=%d frames=%lu rate=%lu sum=%08x\n",(int)info.codec,(unsigned long)count,(unsigned long)info.rate,sum);
 free(out);free(wav);return 0;
}
}
int main(int argc,char** argv){
 if(argc==4&&!strcmp(argv[1],"--bench"))return bench(argv[2],argv[3]);
 if(argc<2||argc>3){puts("usage: audio-control SCRATCH_DIR [ORACLE_DIR]");return 20;}
 wav_tests();mixer_tests();settings_tests();pump_tests(argv[1]);
 if(argc==3)oracle_tests(argv[2]);
 printf("AUDIO %u checks %s\n",checks,failures?"FAIL":"PASS");return failures?20:0;
}
