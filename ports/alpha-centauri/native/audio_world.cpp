/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "audio_world.h"
#include <stdio.h>
#include <string.h>
namespace alpha {
namespace {
const char* const settings_file="alpha-audio.cfg";
bool settings_dirty=false;
}
bool audio_faction_stem(const OriginalWorld& world,size_t index,char* out,size_t cap){
 const auto* def=world.faction_definition(index);if(!def||!out||cap<2)return false;
 size_t n=0;
 for(;n<24&&def->filename[n];++n){unsigned char c=(unsigned char)def->filename[n];if(c>='A'&&c<='Z')c=(unsigned char)(c-'A'+'a');
  if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-')||n+1>=cap)return false;
  out[n]=(char)c;}
 out[n]=0;return n>0;
}
void audio_world_enter(const OriginalWorld& world){
 FILE* f=fopen(settings_file,"rb");bool have=f!=nullptr;if(f)fclose(f);
 settings_dirty=false;
 audio_open("",have?settings_file:nullptr);
 if(!have){
  AudioSettings s=audio_settings();const auto& p=world.game().preferences;
  apply_original_preferences((uint32_t)p.GamePreferences,(uint32_t)p.GameMorePreferences,s);audio_set_settings(s);
 }
 char stem[32]="";int32_t player=world.game().player;
 bool named=player>=0&&player<8&&audio_faction_stem(world,(size_t)player,stem,sizeof(stem));
 bool music=audio_world_music(named?stem:nullptr);
 audio_event(AudioEvent::turn_start);
 auto st=audio_status();
 printf("WORLD AUDIO ready=%d faction=%s music=%d settings=%s status=%s\n",st.ready,named?stem:"-",music,have?settings_file:"save-preferences",st.last_error);fflush(stdout);
}
void audio_world_leave(){
 if(settings_dirty&&!audio_save_settings(settings_file))printf("WORLD AUDIO settings not saved\n");
 settings_dirty=false;audio_close();
}
const char* audio_world_key(unsigned code){
 AudioSettings s=audio_settings();const char* msg=nullptr;
 switch(code){
  case 'm':case 'M':s.levels.music_on=!s.levels.music_on;msg=s.levels.music_on?"Music on (M)":"Music off (M)";break;
  case 'n':case 'N':s.levels.sfx_on=!s.levels.sfx_on;msg=s.levels.sfx_on?"Sound effects on (N)":"Sound effects off (N)";break;
  case 'j':case 'J':s.levels.voice_on=!s.levels.voice_on;msg=s.levels.voice_on?"Voices on (J)":"Voices off (J)";break;
  default:return nullptr;
 }
 audio_set_settings(s);settings_dirty=true;audio_event(AudioEvent::ui_click);
 printf("WORLD AUDIO key=%c music=%d effects=%d voices=%d\n",(char)code,s.levels.music_on,s.levels.sfx_on,s.levels.voice_on);fflush(stdout);
 return msg;
}
}
