/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "land_engagement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static UnitDefinitions units;static Rules tech;static BasicRules basic;static WorldRules worldrules;
static unsigned char*read(const char*p,size_t&n){FILE*f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
static bool word(uint32_t n,unsigned bytes=4){for(unsigned i=0;i<bytes;++i){if(fputc((int)(n&255),stdout)==EOF)return false;n>>=8;}return true;}
static bool vehicle(const OriginalVehicle&v){
#define WIRE_SCALAR(type,name,offset) if(!word((uint32_t)v.name,sizeof(type)))return false;
#define WIRE_ARRAY(type,name,count,offset) for(size_t j=0;j<count;++j)if(!word((uint32_t)v.name[j],sizeof(type)))return false;
#define WIRE_STRING(name,count,offset) if(fwrite(v.name,1,count,stdout)!=count)return false;
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
return true;}
static bool tile(const OriginalMapTile&t){if(!word(t.climate,1)||!word(t.contour,1)||!word(t.val2,1)||!word(t.region,1)||!word(t.visibility,1)||!word(t.val3,1)||!word(t.unknown,1)||!word((uint8_t)t.owner,1)||!word(t.items)||!word(t.landmarks))return false;for(unsigned i=0;i<7;++i)if(!word(t.visible_items[i]))return false;return true;}
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||memcmp(a.current_monuments().data,b.current_monuments().data,a.current_monuments().size)||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||memcmp(&a.game().preferences,&b.game().preferences,sizeof a.game().preferences)||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;for(size_t i=0;i<8;++i)if(memcmp(a.faction(i),b.faction(i),sizeof(OriginalFactionState))||memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;return true;}
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;LandEngagementPlan plan;unsigned accepted=0;size_t last_a=0,last_b=0;EngagementFacts last_facts{};EngagementFacts facts{EngagementObservation::supplied,0,0,false,{61,11}};
 for(unsigned profile=0;profile<2;++profile)for(size_t a=0;a<w.game().vehicle_count;++a)for(size_t b=0;b<w.game().vehicle_count;++b){facts.crt_state=profile?0xffffffffu:0u;facts.incoming_direction=profile?3:0;facts.multiplayer_active=profile!=0;const auto*oldframe=plan.frame();const auto*olds=plan.summary();EngagementSummary old{};if(olds)old=*olds;const auto e=stage_land_engagement(w,units,tech,basic,a,b,facts,plan);if(e!=EngagementError::ok&&(oldframe!=plan.frame()||(olds&&memcmp(&old,plan.summary(),sizeof old))))return 21;const auto*s=e==EngagementError::ok?plan.summary():nullptr;
 uint32_t row[21]{};row[0]=e==EngagementError::ok?0:e==EngagementError::overflow?2:1;if(s){row[1]=(uint32_t)s->strengths.offense;row[2]=(uint32_t)s->strengths.defense;row[3]=(uint32_t)s->strengths.offense_morale;row[4]=(uint32_t)s->strengths.defense_morale;row[5]=s->strengths.terrain;row[6]=s->strengths.defense_multiplier;row[7]=s->strengths.sensor_coverage;row[8]=s->rounds;row[9]=s->draws;row[10]=s->initial_counter;row[11]=s->remaining_counter;row[12]=s->next_crt_state;row[13]=s->attacker_damage;row[14]=s->defender_damage;row[15]=s->attacker_health;row[16]=s->defender_health;row[17]=(uint32_t)s->retreat_direction;row[18]=(uint32_t)s->retreat_x;row[19]=(uint32_t)s->retreat_y;row[20]=(uint32_t)s->boundary;}for(auto v:row)if(!word(v))return 22;
 if(s){++accepted;last_a=a;last_b=b;last_facts=facts;const auto&f=*plan.frame();if(!collateral(w,f))return 23;for(size_t i=0;i<w.game().vehicle_count;++i)if(!vehicle(*f.vehicle(i)))return 22;for(size_t i=0;i<w.game().map.info.tile_count;++i)if(!tile(*f.tile(i)))return 22;
  const auto saved=*s;if(stage_land_engagement(w,units,tech,basic,a,b,facts,plan,1)!=EngagementError::out_of_memory||memcmp(&saved,plan.summary(),sizeof saved))return 24;
 }
 }
 if(w.runtime_seed()!=0x12345678||memcmp(w.original_bytes(),data,n))return 25;
 if(accepted){
  const EngagementSummary saved=*plan.summary();const auto* frame_before=plan.frame();
  const uintptr_t overlaps[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)&units,(uintptr_t)&tech,(uintptr_t)&basic,(uintptr_t)&last_facts};
  for(auto address:overlaps)if(address&&stage_land_engagement(w,units,tech,basic,last_a,last_b,last_facts,alias<LandEngagementPlan>(address))!=EngagementError::bad_state)return 27;
  if(stage_land_engagement(*plan.frame(),units,tech,basic,last_a,last_b,last_facts,plan)!=EngagementError::bad_state||plan.frame()!=frame_before||memcmp(&saved,plan.summary(),sizeof saved))return 28;
  auto* current_vehicle=const_cast<OriginalVehicle*>(w.vehicle(0));current_vehicle->year_end_lurking^=128u;
  auto* current_prototype=const_cast<OriginalPrototype*>(w.prototype(511));current_prototype->name[0]^=1;
  const_cast<uint8_t*>(w.current_monuments().data)[0]^=64u;uint8_t current_history[4096];memcpy(current_history,w.game().bunglist.data,4096);current_history[0]^=32u;const_cast<OriginalGameView&>(w.game()).bunglist={current_history,4096};
  if(stage_land_engagement(w,units,tech,basic,last_a,last_b,last_facts,plan)!=EngagementError::ok||!collateral(w,*plan.frame())||plan.frame()->vehicle(0)->year_end_lurking!=current_vehicle->year_end_lurking||plan.frame()->prototype(511)->name[0]!=current_prototype->name[0])return 29;
  const auto*frame=plan.frame();w.clear();free(text);text=nullptr;if(!frame||frame->runtime_seed()!=0x12345678||memcmp(frame->original_bytes(),data,n)||!frame->vehicle(0)||!frame->prototype(511)||!frame->faction(7)||frame->game().bunglist.size!=4096)return 26;}
 free(data);free(text);return 0;}
