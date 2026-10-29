/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_battle_entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static UnitDefinitions d;static Rules t;static BasicRules r;static WorldRules wr;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
static bool word(uint32_t n){for(unsigned i=0;i<4;++i){if(fputc((int)(n&255),stdout)==EOF)return false;n>>=8;}return true;}
template<class T>static T&at(uintptr_t x){return *reinterpret_cast<T*>(x);}
static uint32_t fingerprint(const OriginalWorld&w){
 uint32_t h=2166136261u;auto add=[&](const void*p,size_t n){const auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};const auto&g=w.game();
 add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle));add(w.base(0),g.base_count*sizeof(OriginalBase));add(w.prototype(0),512*sizeof(OriginalPrototype));add(w.faction(0),8*sizeof(OriginalFactionState));add(w.faction_definition(0),8*sizeof(OriginalFactionDefinition));add(w.current_monuments().data,w.current_monuments().size);add(g.bunglist.data,g.bunglist.size);return h;
}
int main(int argc,char**argv){
 if(argc!=3)return 20;
 size_t nt=0,n=0;auto*text=read(argv[1],nt);auto*data=read(argv[2],n);if(!text||!data)return 20;
 OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto ed=w.game().data_version<11?Variant::base:Variant::alien_crossfire;
 if(read_technologies(text,nt,t)!=Error::ok||read_unit_definitions(text,nt,t,ed,d)!=Error::ok||read_basic_rules(text,nt,t,ed,r,wr)!=Error::ok)return 20;
 const auto current_before=fingerprint(w);
 const size_t bytes=w.game().vehicle_count*sizeof(OriginalVehicle);auto*actors=(unsigned char*)malloc(bytes);if(bytes&&!actors)return 20;if(bytes)memcpy(actors,w.vehicle(0),bytes);
 WorldBattleEntryInspection last{};WorldBattleEntryRuntime f{BattleEntryObservation::supplied,2,0,1,0,0};unsigned guards=0,accepted=0;
 for(int count:{0,3,4})for(size_t a=0;a<w.game().vehicle_count;++a)for(size_t b=0;b<w.game().vehicle_count;++b){
  f.attacker_report_count=count;WorldBattleEntryInspection out;memset(&out,0xa5,sizeof out);unsigned char before[sizeof out];memcpy(before,&out,sizeof out);
  auto e=inspect_world_battle_entry(w,d,t,r,a,b,f,out);if(e!=WorldBattleEntryError::ok&&memcmp(before,&out,sizeof out))return 21;
  uint32_t row[20]{};row[0]=(uint32_t)e;if(e==WorldBattleEntryError::ok){++accepted;last=out;row[1]=(uint32_t)out.before_hasty.offense;row[2]=(uint32_t)out.before_hasty.defense;row[3]=(uint32_t)out.before_hasty.offense_morale;row[4]=(uint32_t)out.before_hasty.defense_morale;row[5]=out.before_hasty.terrain;row[6]=out.before_hasty.defense_multiplier;row[7]=out.before_hasty.sensor_coverage;row[8]=out.speed;row[9]=(uint32_t)out.charge.charge;row[10]=out.charge.speed_used;row[11]=out.hasty.attacker_state;row[12]=out.hasty.defender_state;row[13]=out.hasty.moves_spent;row[14]=out.hasty.scaled;row[15]=(uint32_t)out.hasty.offense;row[16]=(uint32_t)out.hasty.penalty;row[17]=(uint32_t)out.hasty.report_count;row[18]=out.hasty.string_lookup_required;row[19]=out.hasty.report_append_required;}
  for(auto x:row)if(!word(x))return 22;
 }
 if(accepted){
  const uintptr_t addresses[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)w.game().bunglist.data,(uintptr_t)&d,(uintptr_t)&t,(uintptr_t)&r,(uintptr_t)&f};
  for(auto x:addresses)if(x){const auto aligned=(x+alignof(WorldBattleEntryInspection)-1)&~uintptr_t(alignof(WorldBattleEntryInspection)-1);if(inspect_world_battle_entry(w,d,t,r,0,1,f,at<WorldBattleEntryInspection>(aligned))!=WorldBattleEntryError::bad_state)return 23;++guards;}
 }
 constexpr uintptr_t bad=4096;
 if(inspect_world_battle_entry(w,at<UnitDefinitions>(bad),t,r,0,1,f,at<WorldBattleEntryInspection>(bad))!=WorldBattleEntryError::bad_state||inspect_world_battle_entry(w,d,at<Rules>(bad),r,0,1,f,at<WorldBattleEntryInspection>(bad))!=WorldBattleEntryError::bad_state||inspect_world_battle_entry(w,d,t,at<BasicRules>(bad),0,1,f,at<WorldBattleEntryInspection>(bad))!=WorldBattleEntryError::bad_state||inspect_world_battle_entry(w,d,t,r,0,1,at<WorldBattleEntryRuntime>(bad),at<WorldBattleEntryInspection>(bad))!=WorldBattleEntryError::bad_state)return 24;
 guards+=4;
 WorldBattleEntryInspection output=last;
 for(unsigned k=0;k<10;++k){auto x=f;if(k==0)x.observation=BattleEntryObservation::missing;if(k==1)x.attack_flags=0;if(k==2)x.mode=2;if(k==3)x.execution_argument=0;if(k==4)x.multiplayer_active=1;if(k==5)x.multiplayer_active=2;if(k==6)x.attacker_report_count=-1;if(k==7)x.attacker_report_count=5;if(k==8)x.attack_flags=3;if(k==9)x.execution_argument=-1;if(inspect_world_battle_entry(w,d,t,r,0,1,x,output)==WorldBattleEntryError::ok||memcmp(&last,&output,sizeof output))return 25;++guards;}
 if(w.runtime_seed()!=0x12345678||fingerprint(w)!=current_before||memcmp(w.original_bytes(),data,n)||(bytes&&memcmp(w.vehicle(0),actors,bytes)))return 26;
 if(!word(guards))return 22;
 free(actors);free(data);free(text);return 0;
}
