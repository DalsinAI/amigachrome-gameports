/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "promoted_land_body.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
alignas(PromotedLandResolutionPlan) static UnitDefinitions units;alignas(PromotedLandResolutionPlan) static Rules tech;alignas(PromotedLandResolutionPlan) static BasicRules basic;static WorldRules worldrules;
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
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||memcmp(a.current_monuments().data,b.current_monuments().data,a.current_monuments().size)||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||([&](){auto pref=a.game().preferences;pref.VehCount=(int32_t)b.game().vehicle_count;return memcmp(&pref,&b.game().preferences,sizeof pref);})()||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;for(size_t i=0;i<8;++i)if(memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;return true;}
static uint32_t hashbytes(uint32_t h,const void*p,size_t n){auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hashbytes(h,w.original_bytes(),w.original_size());h=hashbytes(h,w.current_monuments().data,w.current_monuments().size);h=hashbytes(h,w.game().bunglist.data,w.game().bunglist.size);h=hashbytes(h,&w.game().preferences,sizeof w.game().preferences);h=hashbytes(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hashbytes(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hashbytes(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hashbytes(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hashbytes(h,w.faction(0),8*sizeof(OriginalFactionState));return hashbytes(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static PromotedBodyError stage_wrapper(const OriginalWorld&w,const UnitDefinitions&u,const Rules&t,const BasicRules&b,size_t a,size_t d,const LandResolutionFacts&f,PromotedLandResolutionPlan&out,size_t limit=64u*1024u*1024u){const PromotionRuntime run{EngagementObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,f.combat.crt_state&&w.vehicle(a)?(int32_t)w.vehicle(a)->faction_id:-1};return stage_promoted_land_body(w,u,t,b,a,d,f,run,out,limit);}
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;
const auto nv=w.game().vehicle_count;const auto before=fingerprint(w);PromotedLandResolutionPlan plan;unsigned middle_guard=0;unsigned accepted=0;size_t last_a=0,last_b=0;alignas(PromotedLandResolutionPlan) LandResolutionFacts last{};
for(unsigned profile=0;profile<2;++profile)for(size_t a=0;a<nv;++a)for(size_t b=0;b<nv;++b){
 alignas(PromotedLandResolutionPlan) LandResolutionFacts facts{{EngagementObservation::supplied,profile?0xffffffffu:0u,0,false,{61,11}},{RemovalObservation::supplied,false,false,false,{0,(int32_t)a,(int32_t)b,(int32_t)nv},(int32_t)b,0,0,0},EngagementObservation::supplied,6,7,{{profile==0,2147483647},{true,2147483647},{profile==0,-5}}};
 const auto*oldframe=plan.frame();PromotedBodySummary old{};if(plan.summary())old=*plan.summary();const auto e=stage_wrapper(w,units,tech,basic,a,b,facts,plan);
 if(e!=PromotedBodyError::ok&&(oldframe!=plan.frame()||(oldframe&&memcmp(&old,plan.summary(),sizeof old))))return 21;
 uint32_t row[21]{};row[0]=e==PromotedBodyError::ok?0:e==PromotedBodyError::overflow?2:e==PromotedBodyError::selection_random?3:e==PromotedBodyError::promotion_random?4:e==PromotedBodyError::promotion_notification?5:1;
 const auto*s=e==PromotedBodyError::ok?plan.summary():nullptr;
 if(s){const auto&v=s->body.engagement;row[1]=(uint32_t)v.strengths.offense;row[2]=(uint32_t)v.strengths.defense;row[3]=(uint32_t)v.strengths.offense_morale;row[4]=(uint32_t)v.strengths.defense_morale;row[5]=v.strengths.terrain;row[6]=v.strengths.defense_multiplier;row[7]=v.strengths.sensor_coverage;row[8]=v.rounds;row[9]=v.draws;row[10]=v.initial_counter;row[11]=v.remaining_counter;row[12]=v.next_crt_state;row[13]=v.attacker_damage;row[14]=v.defender_damage;row[15]=v.attacker_health;row[16]=v.defender_health;row[17]=(uint32_t)v.retreat_direction;row[18]=(uint32_t)v.retreat_x;row[19]=(uint32_t)v.retreat_y;row[20]=(uint32_t)v.boundary;}
 for(auto v:row)if(!word(v))return 22;
 if(s){++accepted;last_a=a;last_b=b;last=facts;const auto&f=*plan.frame();if(!collateral(w,f))return 23;
  for(size_t j=0;j<8;++j){auto actual=*f.faction(j);for(size_t id=0;id<512;++id){actual.units_active[id]=w.faction(j)->units_active[id];actual.units_lost[id]=w.faction(j)->units_lost[id];}actual.total_combat_units=w.faction(j)->total_combat_units;actual.unk_101=w.faction(j)->unk_101;if(memcmp(&actual,w.faction(j),sizeof actual))return 24;}
  const uint32_t detail[]={(uint32_t)s->body.kind,(uint32_t)s->body.casualty_before,(uint32_t)s->body.counterpart_before,(uint32_t)s->body.argument6,(uint32_t)s->body.source_casualty_return,(uint32_t)s->body.attacker_after,(uint32_t)s->body.defender_after,f.game().vehicle_count,plan.runtime()?1u:0u};for(auto v:detail)if(!word(v))return 22;
  for(size_t j=0;j<f.game().vehicle_count;++j)if(!vehicle(*f.vehicle(j)))return 22;
  for(size_t j=0;j<w.game().map.info.tile_count;++j)if(!tile(*f.tile(j)))return 22;
  for(size_t j=0;j<8;++j){const auto&fs=*f.faction(j);if(fwrite(fs.units_active,1,512,stdout)!=512)return 22;for(auto v:fs.units_lost)if(!word(v,2))return 22;if(!word((uint32_t)fs.total_combat_units)||!word((uint32_t)fs.unk_101)||!word((uint32_t)fs.energy_credits))return 22;}
  int32_t runtime[8]{};if(const auto*r=plan.runtime()){for(unsigned j=0;j<4;++j)runtime[j]=r->cursor[j];runtime[4]=r->iteration_cursor;runtime[5]=r->lift_vehicle;runtime[6]=r->lift_x;runtime[7]=r->lift_y;}for(auto v:runtime)if(!word((uint32_t)v))return 22;
  const auto&pools=*plan.accumulators();const CasualtyAccumulator*list[]={&pools.killed,&pools.captured,&pools.native_reward};for(const auto*p:list)if(!word(p->present?1:0)||!word((uint32_t)p->value))return 22;
  const auto&se=s->selection;const auto&pr=s->promotion;const uint32_t promotion[]={(uint32_t)se.intent,se.recipient,(uint32_t)se.attacker_strength,(uint32_t)se.defender_strength,s->helper_called?1u:0u,pr.raw_before,pr.raw_after,pr.effective_before,pr.queried_morale?1u:0u,pr.promoted?1u:0u,s->initial_attacker_health,s->initial_defender_health};for(auto v:promotion)if(!word(v))return 22;
  const auto saved=*s;if(stage_wrapper(w,units,tech,basic,a,b,facts,plan,1)!=PromotedBodyError::out_of_memory||memcmp(&saved,plan.summary(),sizeof saved))return 25;
  if(!middle_guard&&s->body.engagement.boundary!=EngagementBoundary::retreated){
   const size_t budget=sizeof(PromotedLandResolutionPlan)+f.memory_bytes();
   LandEngagementPlan first;
   if(stage_land_engagement(w,units,tech,basic,a,b,facts.combat,first,budget-(sizeof(PromotedLandResolutionPlan)-sizeof(LandEngagementPlan)))!=EngagementError::ok)return 31;
   if(stage_wrapper(w,units,tech,basic,a,b,facts,plan,budget)!=PromotedBodyError::out_of_memory||memcmp(&saved,plan.summary(),sizeof saved)||fingerprint(*plan.frame())!=fingerprint(f))return 32;
   middle_guard=1;
  }
 }
 if(fingerprint(w)!=before)return 26;
}
if(accepted){const auto saved=*plan.summary();const auto*p=plan.frame();const auto staged=fingerprint(*p);const void*spots[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),w.game().bunglist.data,&units,&tech,&basic,&last};
 for(auto spot:spots)if(spot){auto*out=reinterpret_cast<PromotedLandResolutionPlan*>(((uintptr_t)spot+alignof(PromotedLandResolutionPlan)-1)&~(uintptr_t)(alignof(PromotedLandResolutionPlan)-1));if(stage_wrapper(w,units,tech,basic,last_a,last_b,last,*out)!=PromotedBodyError::bad_state)return 27;}
 if(stage_wrapper(*p,units,tech,basic,last_a,last_b,last,plan)!=PromotedBodyError::bad_state||memcmp(&saved,plan.summary(),sizeof saved)||fingerprint(*p)!=staged)return 28;
 for(unsigned j=0;j<3;++j){auto f=last;if(j==0)f.caller_arguments=EngagementObservation::missing;if(j==1)f.combat.multiplayer_active=true;if(j==2)f.removal.logging_enabled=true;const auto e=stage_wrapper(w,units,tech,basic,last_a,last_b,f,plan);if(e!=(j?PromotedBodyError::unsupported:PromotedBodyError::bad_state)||plan.frame()!=p||memcmp(&saved,plan.summary(),sizeof saved)||fingerprint(*p)!=staged)return 29;}
 alignas(PromotedLandResolutionPlan) PromotionRuntime run{EngagementObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,last.combat.crt_state?(int32_t)w.vehicle(last_a)->faction_id:-1};
 for(unsigned j=0;j<4;++j){auto r=run;if(j==0)r.observation=EngagementObservation::missing;if(j==1)r.current_player=-2;if(j==2)r.current_player=8;if(j==3)r.edition=w.game().data_version>=11?Variant::base:Variant::alien_crossfire;const auto e=stage_promoted_land_body(w,units,tech,basic,last_a,last_b,last,r,plan);if(e!=(j==3?PromotedBodyError::unsupported:PromotedBodyError::bad_state)||fingerprint(*p)!=staged||memcmp(&saved,plan.summary(),sizeof saved))return 33;}
 const void*poisoned[]={&units,&tech,&basic,&last,&run};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof last,sizeof run};
 for(unsigned j=0;j<5;++j){const auto before=hashbytes(2166136261u,poisoned[j],sizes[j]);
#ifdef __SANITIZE_ADDRESS__
  __asan_poison_memory_region(poisoned[j],sizes[j]);
#endif
  const auto e=stage_promoted_land_body(w,units,tech,basic,last_a,last_b,last,run,alias<PromotedLandResolutionPlan>((uintptr_t)poisoned[j]));
#ifdef __SANITIZE_ADDRESS__
  __asan_unpoison_memory_region(poisoned[j],sizes[j]);
#endif
  if(e!=PromotedBodyError::bad_state||hashbytes(2166136261u,poisoned[j],sizes[j])!=before||fingerprint(*p)!=staged||memcmp(&saved,plan.summary(),sizeof saved))return 34;
 }
 w.clear();memset(data,0,n);free(data);data=nullptr;memset(text,0,nt);free(text);text=nullptr;memset(&units,0,sizeof units);memset(&tech,0,sizeof tech);memset(&basic,0,sizeof basic);if(fingerprint(*p)!=staged)return 30;
}fprintf(stderr,"MIDDLE_CLONE_GUARD %u\n",middle_guard);free(data);free(text);return 0;}
