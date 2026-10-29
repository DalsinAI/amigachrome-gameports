/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_events.h"
#include "leader_speech.h"
#include <stdio.h>
#include <string.h>
namespace alpha {
namespace {
// Defaults name only fx files whose use is attested by an open upstream. Events
// without an attested original file stay silent until the owner maps them in
// alpha-audio.cfg (tools/prepare_alpha_audio.py lists the local fx folder).
const char* const glsmac="GLSMAC 0fe9ef16 GLSMAC_data/default/ui (file names only)";
const AudioEventInfo events[audio_event_count]={
 {"ui_click","ok.wav",glsmac},
 {"menu_open","menu up.wav",glsmac},
 {"menu_close","menu down.wav",glsmac},
 {"panel_slide","menu out.wav",glsmac},
 {"command_menu","mmenu.wav",glsmac},
 {"quit_prompt","CPU please don't go.wav",glsmac},
 {"unit_move",nullptr,"unattested: map in alpha-audio.cfg"},
 {"unit_attack",nullptr,"unattested: map in alpha-audio.cfg"},
 {"combat_hit",nullptr,"unattested: map in alpha-audio.cfg"},
 {"psi_attack","psi attack.wav","GLSMAC 0fe9ef16 units/animations ATTACK_PSI"},
 {"unit_destroyed",nullptr,"unattested: map in alpha-audio.cfg"},
 {"psi_destroyed","psi blow.wav","GLSMAC 0fe9ef16 units/animations DEATH_PSI"},
 {"base_built",nullptr,"unattested: map in alpha-audio.cfg"},
 {"facility_complete",nullptr,"voice voices/fac<N>.mp3 (Thinker base.cpp); effect unattested"},
 {"tech_discovered",nullptr,"voice voices/tech<ID>.mp3 (NewTechWin_exec); effect unattested"},
 {"turn_start",nullptr,"unattested: map in alpha-audio.cfg"},
 {"turn_complete","cpu turn complete.wav",glsmac},
};
// Thinker dabe62f2 src/game.cpp load_music defaults (faction file -> AMB set).
struct FactionSet { const char* faction; const char* set; };
const FactionSet faction_sets[]={{"gaians","gset1.amb"},{"hive","mset1.amb"},{"univ","uset2.amb"},{"morgan","mset1.amb"},{"spartans","sset1.amb"},{"believe","bset1.amb"},{"peace","gset1.amb"}};
const char* const sets[]={"aset1","gset1","mset1","uset2","sset1","bset1"};
const char* const default_set="aset1.amb";
bool same_nocase(const char* a,size_t an,const char* b){
 size_t bn=strlen(b);if(an!=bn)return false;
 for(size_t i=0;i<an;++i){unsigned x=(unsigned char)a[i],y=(unsigned char)b[i];if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x!=y)return false;}
 return true;
}
bool parse_percent(const char* v,size_t n,uint8_t& out){
 if(!n||n>3)return false;
 unsigned x=0;for(size_t i=0;i<n;++i){if(v[i]<'0'||v[i]>'9')return false;x=x*10+(unsigned)(v[i]-'0');}
 if(x>100)return false;
 out=(uint8_t)x;return true;
}
bool parse_switch(const char* v,size_t n,bool& out){
 if(same_nocase(v,n,"on")||same_nocase(v,n,"1")||same_nocase(v,n,"yes")){out=true;return true;}
 if(same_nocase(v,n,"off")||same_nocase(v,n,"0")||same_nocase(v,n,"no")){out=false;return true;}
 return false;
}
// A mapped fx name: a plain basename ending in .wav, no path separators.
bool valid_fx_name(const char* v,size_t n){
 if(n<5||n>=audio_name_bytes||!same_nocase(v+n-4,4,".wav"))return false;
 for(size_t i=0;i<n;++i){unsigned char c=(unsigned char)v[i];if(c<32||c==127||c=='/'||c=='\\'||c==':')return false;}
 return !(v[0]=='.');
}
bool put(char* out,size_t cap,size_t& at,const char* text){size_t n=strlen(text);if(n>=cap-at)return false;memcpy(out+at,text,n);at+=n;out[at]=0;return true;}
}
AudioSettings::AudioSettings(){memset(mapped,0,sizeof(mapped));memset(file,0,sizeof(file));}
const AudioEventInfo& audio_event_info(AudioEvent e){return events[(size_t)e<audio_event_count?(size_t)e:0];}
bool audio_event_from_key(const char* key,size_t n,AudioEvent& out){
 for(size_t i=0;i<audio_event_count;++i)if(same_nocase(key,n,events[i].key)){out=(AudioEvent)i;return true;}
 return false;
}
unsigned parse_audio_settings(const char* text,size_t length,AudioSettings& result){
 if(!text&&length)return 1;
 AudioSettings s=result;unsigned line=0;
 for(size_t pos=0;pos<length;){
  ++line;size_t end=pos;while(end<length&&text[end]!='\n')++end;
  size_t a=pos,b=end;pos=end+1;
  while(a<b&&(text[a]==' '||text[a]=='\t'))++a;
  while(b>a&&(text[b-1]==' '||text[b-1]=='\t'||text[b-1]=='\r'))--b;
  if(a==b||text[a]=='#'||text[a]==';')continue;
  size_t eq=a;while(eq<b&&text[eq]!='=')++eq;if(eq==b)return line;
  size_t ke=eq;while(ke>a&&(text[ke-1]==' '||text[ke-1]=='\t'))--ke;size_t vs=eq+1;while(vs<b&&(text[vs]==' '||text[vs]=='\t'))++vs;
  const char* k=text+a;size_t kn=ke-a;const char* v=text+vs;size_t vn=b-vs;bool ok=false;
  if(same_nocase(k,kn,"sound_effects"))ok=parse_switch(v,vn,s.levels.sfx_on);
  else if(same_nocase(k,kn,"music"))ok=parse_switch(v,vn,s.levels.music_on);
  else if(same_nocase(k,kn,"voices"))ok=parse_switch(v,vn,s.levels.voice_on);
  else if(same_nocase(k,kn,"voice_stops_on_close"))ok=parse_switch(v,vn,s.voice_stops_on_close);
  else if(same_nocase(k,kn,"master_volume"))ok=parse_percent(v,vn,s.levels.master);
  else if(same_nocase(k,kn,"effects_volume"))ok=parse_percent(v,vn,s.levels.sfx);
  else if(same_nocase(k,kn,"music_volume"))ok=parse_percent(v,vn,s.levels.music);
  else if(same_nocase(k,kn,"voice_volume"))ok=parse_percent(v,vn,s.levels.voice);
  else if(same_nocase(k,kn,"music_under_voice"))ok=parse_percent(v,vn,s.levels.music_duck);
  else if(kn>4&&same_nocase(k,4,"map.")){
   AudioEvent e;if(audio_event_from_key(k+4,kn-4,e)){size_t i=(size_t)e;
    if(!vn||same_nocase(v,vn,"none")){s.mapped[i]=true;s.file[i][0]=0;ok=true;}
    else if(same_nocase(v,vn,"default")){s.mapped[i]=false;s.file[i][0]=0;ok=true;}
    else if(valid_fx_name(v,vn)){s.mapped[i]=true;memcpy(s.file[i],v,vn);s.file[i][vn]=0;ok=true;}}
  }
  if(!ok)return line;
 }
 result=s;return 0;
}
size_t format_audio_settings(const AudioSettings& s,char* out,size_t cap){
 if(!out||!cap)return 0;
 size_t at=0;out[0]=0;char line[160];
 const auto& l=s.levels;
 snprintf(line,sizeof(line),"# Alpha Centauri native audio settings\nsound_effects=%s\nmusic=%s\nvoices=%s\nvoice_stops_on_close=%s\n",l.sfx_on?"on":"off",l.music_on?"on":"off",l.voice_on?"on":"off",s.voice_stops_on_close?"on":"off");
 if(!put(out,cap,at,line))return 0;
 snprintf(line,sizeof(line),"master_volume=%u\neffects_volume=%u\nmusic_volume=%u\nvoice_volume=%u\nmusic_under_voice=%u\n",l.master,l.sfx,l.music,l.voice,l.music_duck);
 if(!put(out,cap,at,line))return 0;
 for(size_t i=0;i<audio_event_count;++i)if(s.mapped[i]){
  snprintf(line,sizeof(line),"map.%s=%s\n",events[i].key,s.file[i][0]?s.file[i]:"none");if(!put(out,cap,at,line))return 0;
 }
 return at;
}
void apply_original_preferences(uint32_t p,uint32_t more,AudioSettings& s){
 s.levels.sfx_on=(p&0x400u)!=0;s.levels.music_on=(p&0x800u)!=0;s.levels.voice_on=(more&0x4000u)!=0;s.voice_stops_on_close=(more&0x20000u)!=0;
}
const char* audio_event_file(const AudioSettings& s,AudioEvent e){
 size_t i=(size_t)e;if(i>=audio_event_count)return nullptr;
 if(s.mapped[i])return s.file[i][0]?s.file[i]:nullptr;
 return events[i].file;
}
bool faction_voice_path(const char* stem,bool expansion,char* out,size_t cap){
 int i=find_leader_speech(stem,expansion);if(i<0||!out)return false;
 int n=snprintf(nullptr,0,"voices/%s.wav",leader_speech((size_t)i)->stem);if(n<0||(size_t)n>=cap)return false;
 snprintf(out,cap,"voices/%s.wav",leader_speech((size_t)i)->stem);return true;
}
bool technology_voice_path(int id,char* out,size_t cap){
 // 0..88 is the original technology table; 87 is the disabled user technology.
 if(id<0||id>88||id==87||!out||cap<20)return false;
 snprintf(out,cap,"voices/tech%d.wav",id);return true;
}
bool facility_voice_path(int id,char* out,size_t cap){
 // Thinker base.cpp: only ordinary facilities (< SP_ID_First 70) are voiced.
 // FAC_SKY_HYDRO_LAB..FAC_STOCKPILE_ENERGY (65..69) use id-31; the SMAX
 // facilities FAC_COVERT_OPS_CENTER..FAC_GEOSYNC_SURVEY_POD (34..41) add "0".
 if(id<1||id>=70||!out||cap<20)return false;
 bool extra=id>=65&&id<=69,smacx=id>=34&&id<=41;
 snprintf(out,cap,"voices/fac%d%s.wav",extra?id-31:id,smacx?"0":"");return true;
}
const char* faction_music_set(const char* stem){
 if(stem)for(const auto& f:faction_sets)if(same_nocase(stem,strlen(stem),f.faction))return f.set;
 return default_set;
}
size_t music_set_count(){return sizeof(sets)/sizeof(sets[0]);}
const char* music_set(size_t i){return i<music_set_count()?sets[i]:nullptr;}
size_t MusicPlaylist::build(const char* stem,AudioExistsFn exists,void* ctx){
 count=cursor=0;if(!exists)return 0;
 char first[16];snprintf(first,sizeof(first),"%s",faction_music_set(stem));char* dot=strchr(first,'.');if(dot)*dot=0;
 auto add_set=[&](const char* set){
  char path[audio_name_bytes];
  for(int n=0;n<=9&&count<max_tracks;++n){
   if(n)snprintf(path,sizeof(path),"music/%s-%d.wav",set,n);else snprintf(path,sizeof(path),"music/%s.wav",set);
   bool dup=false;for(size_t j=0;j<count;++j)if(!strcmp(track[j],path))dup=true;
   if(!dup&&exists(ctx,path)){memcpy(track[count],path,strlen(path)+1);++count;}
  }
 };
 add_set(first);                                 // the faction's own score set
 if(!count)add_set("aset1");                     // the original default set
 for(size_t i=0;!count&&i<music_set_count();++i)add_set(sets[i]); // any prepared set
 return count;
}
const char* MusicPlaylist::next(){if(!count)return nullptr;const char* t=track[cursor%count];cursor=(cursor+1)%count;return t;}
}
