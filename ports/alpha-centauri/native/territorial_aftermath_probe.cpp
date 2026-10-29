/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "territorial_aftermath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
alignas(TerritorialBattleAftermathPlan) static UnitDefinitions units;alignas(TerritorialBattleAftermathPlan) static Rules tech;alignas(TerritorialBattleAftermathPlan) static BasicRules basic;static WorldRules worldrules;
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
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||([&](){auto pref=a.game().preferences;pref.VehCount=(int32_t)b.game().vehicle_count;return memcmp(&pref,&b.game().preferences,sizeof pref);})()||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;for(size_t i=0;i<8;++i)if(memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;return true;}
static uint32_t hashbytes(uint32_t h,const void*p,size_t n){auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hashbytes(h,w.original_bytes(),w.original_size());h=hashbytes(h,w.current_monuments().data,w.current_monuments().size);h=hashbytes(h,w.game().bunglist.data,w.game().bunglist.size);h=hashbytes(h,&w.game().preferences,sizeof w.game().preferences);h=hashbytes(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hashbytes(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hashbytes(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hashbytes(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hashbytes(h,w.faction(0),8*sizeof(OriginalFactionState));return hashbytes(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static void emit_word(uint32_t v){(void)word(v);}
static void writestate(const BattleBookkeepingState&s){for(auto v:s.wins)emit_word(v);for(auto v:s.losses)emit_word(v);for(auto&r:s.combat_total)for(auto v:r)emit_word(v);for(auto&r:s.combat_count)for(auto v:r)emit_word(v);for(auto&r:s.enemy_destroyed){emit_word(r.year);emit_word(r.opponent);emit_word(r.leader);emit_word(r.awarded);emit_word(r.rank);emit_word(r.focus);}}

static BattleAftermathRuntime runfacts(const OriginalWorld&w,size_t a,const LandResolutionFacts&facts){BattleAftermathRuntime run{BookkeepingObservation::supplied,w.game().data_version>=11?BookkeepingEdition::smax:BookkeepingEdition::smac,facts.combat.crt_state&&w.vehicle(a)?(int32_t)w.vehicle(a)->faction_id:-1,0,0,0x7fffffffu,{},{}};for(size_t i=0;i<8;++i){run.wins[i]=0x7fffffffu+(uint32_t)i;run.losses[i]=0xffffffffu-(uint32_t)i;}return run;}
static TerritoryRuntime diplomacyfacts(const OriginalWorld&w,size_t a,size_t b,const LandResolutionFacts&f){const auto*v=w.vehicle(b);const auto*att=w.vehicle(a);if(!v||!att)return {TerritoryObservation::missing,Variant::base,0,0,0,0,0};const auto*t=w.tile(((size_t)v->y*w.game().map.info.width+(uint32_t)v->x)/2);return {TerritoryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,f.combat.crt_state?0xffffffffu:(uint32_t)(int32_t)t->owner,f.combat.crt_state?0xffffffffu:(uint32_t)w.vehicle(a)->faction_id,f.combat.crt_state?0x7fffffffu:0x80000000u,0,0};}
static PromotedAftermathError stage_test(const OriginalWorld&w,const UnitDefinitions&u,const Rules&t,const BasicRules&b,size_t a,size_t d,const LandResolutionFacts&f,TerritorialBattleAftermathPlan&out,size_t limit=64u*1024u*1024u){const auto r=runfacts(w,a,f);return stage_territorial_aftermath(w,u,t,b,a,d,f,r,diplomacyfacts(w,a,d,f),out,limit);}

int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,nt=0;auto*data=read(argv[2],n);auto*text=read(argv[1],nt);if(!data||!text)return 20;OriginalWorld w;if(w.load(data,n,0x12345678)!=WorldLoadError::ok)return 20;auto variant=w.game().data_version<11?Variant::base:Variant::alien_crossfire;if(read_technologies(text,nt,tech)!=Error::ok||read_unit_definitions(text,nt,tech,variant,units)!=Error::ok||read_basic_rules(text,nt,tech,variant,basic,worldrules)!=Error::ok)return 20;
const auto nv=w.game().vehicle_count;const auto before=fingerprint(w);TerritorialBattleAftermathPlan plan;unsigned middle_guard=0,runtime_guards=0;unsigned accepted=0;size_t last_a=0,last_b=0;alignas(TerritorialBattleAftermathPlan) LandResolutionFacts last{};
for(unsigned profile=0;profile<2;++profile)for(size_t a=0;a<nv;++a)for(size_t b=0;b<nv;++b){
 LandResolutionFacts facts{{EngagementObservation::supplied,profile?0xffffffffu:0u,0,false,{61,11}},{RemovalObservation::supplied,false,false,false,{0,(int32_t)a,(int32_t)b,(int32_t)nv},(int32_t)b,0,0,0},EngagementObservation::supplied,6,7,{{true,0},{true,0},{true,0}}};
 const auto*oldframe=plan.frame();PromotedBodySummary old{};if(plan.summary())old=*plan.summary();const auto e=stage_test(w,units,tech,basic,a,b,facts,plan);
 if(e!=PromotedAftermathError::ok&&(oldframe!=plan.frame()||(oldframe&&memcmp(&old,plan.summary(),sizeof old))))return 21;
 uint32_t row[21]{};row[0]=e==PromotedAftermathError::ok?0:e==PromotedAftermathError::overflow?2:e==PromotedAftermathError::monument_ui?6:e==PromotedAftermathError::selection_random?3:e==PromotedAftermathError::promotion_random?4:e==PromotedAftermathError::promotion_notification?5:1;
 const auto*s=e==PromotedAftermathError::ok?plan.summary():nullptr;
 if(s){const auto&v=s->body.engagement;row[1]=(uint32_t)v.strengths.offense;row[2]=(uint32_t)v.strengths.defense;row[3]=(uint32_t)v.strengths.offense_morale;row[4]=(uint32_t)v.strengths.defense_morale;row[5]=v.strengths.terrain;row[6]=v.strengths.defense_multiplier;row[7]=v.strengths.sensor_coverage;row[8]=v.rounds;row[9]=v.draws;row[10]=v.initial_counter;row[11]=v.remaining_counter;row[12]=v.next_crt_state;row[13]=v.attacker_damage;row[14]=v.defender_damage;row[15]=v.attacker_health;row[16]=v.defender_health;row[17]=(uint32_t)v.retreat_direction;row[18]=(uint32_t)v.retreat_x;row[19]=(uint32_t)v.retreat_y;row[20]=(uint32_t)v.boundary;}
 for(auto v:row)if(!word(v))return 22;
 if(s){++accepted;last_a=a;last_b=b;last=facts;const auto&f=*plan.frame();if(!collateral(w,f))return 23;
  for(size_t j=0;j<8;++j){auto actual=*f.faction(j);for(size_t id=0;id<512;++id){actual.units_active[id]=w.faction(j)->units_active[id];actual.units_lost[id]=w.faction(j)->units_lost[id];}actual.total_combat_units=w.faction(j)->total_combat_units;actual.unk_101=w.faction(j)->unk_101;for(size_t k=0;k<8;++k){actual.diplo_combat_total[k]=w.faction(j)->diplo_combat_total[k];actual.diplo_combat_count[k]=w.faction(j)->diplo_combat_count[k];}const auto&ts=*plan.territorial_summary();if(ts.effect.applied){if(j==(size_t)ts.attacker_faction)actual.diplo_gifts[ts.owner]=w.faction(j)->diplo_gifts[ts.owner];if(j==(size_t)ts.owner)actual.diplo_friction[ts.attacker_faction]=w.faction(j)->diplo_friction[ts.attacker_faction];}if(memcmp(&actual,w.faction(j),sizeof actual))return 24;}
  const uint32_t detail[]={(uint32_t)s->body.kind,(uint32_t)s->body.casualty_before,(uint32_t)s->body.counterpart_before,(uint32_t)s->body.argument6,(uint32_t)s->body.source_casualty_return,(uint32_t)s->body.attacker_after,(uint32_t)s->body.defender_after,f.game().vehicle_count,plan.runtime()?1u:0u};for(auto v:detail)if(!word(v))return 22;
  for(size_t j=0;j<f.game().vehicle_count;++j)if(!vehicle(*f.vehicle(j)))return 22;
  for(size_t j=0;j<w.game().map.info.tile_count;++j)if(!tile(*f.tile(j)))return 22;
  for(size_t j=0;j<8;++j){const auto&fs=*f.faction(j);if(fwrite(fs.units_active,1,512,stdout)!=512)return 22;for(auto v:fs.units_lost)if(!word(v,2))return 22;if(!word((uint32_t)fs.total_combat_units)||!word((uint32_t)fs.unk_101)||!word((uint32_t)fs.energy_credits))return 22;}
  int32_t runtime[8]{};if(const auto*r=plan.runtime()){for(unsigned j=0;j<4;++j)runtime[j]=r->cursor[j];runtime[4]=r->iteration_cursor;runtime[5]=r->lift_vehicle;runtime[6]=r->lift_x;runtime[7]=r->lift_y;}for(auto v:runtime)if(!word((uint32_t)v))return 22;
  const auto&pools=*plan.accumulators();const CasualtyAccumulator*list[]={&pools.killed,&pools.captured,&pools.native_reward};for(const auto*p:list)if(!word(p->present?1:0)||!word((uint32_t)p->value))return 22;
  const auto&se=s->selection;const auto&pr=s->promotion;const uint32_t promotion[]={(uint32_t)se.intent,se.recipient,(uint32_t)se.attacker_strength,(uint32_t)se.defender_strength,s->helper_called?1u:0u,pr.raw_before,pr.raw_after,pr.effective_before,pr.queried_morale?1u:0u,pr.promoted?1u:0u,s->initial_attacker_health,s->initial_defender_health};for(auto v:promotion)if(!word(v))return 22;
  const auto&book=*plan.bookkeeping();writestate(book.before_cleanup);writestate(book.after_cleanup);if(!word(book.winner)||!word(book.loser)||!word(book.monument_changed)||fwrite(f.current_monuments().data,1,0x27a0,stdout)!=0x27a0)return 22;
  const auto&ts=*plan.territorial_summary();const auto&e=ts.effect;const uint32_t territorial[] = {ts.tile_index,(uint32_t)ts.owner,(uint32_t)ts.attacker_faction,(uint32_t)ts.defender_faction,e.applied,(uint32_t)e.gift_before,(uint32_t)e.gift_after,(uint32_t)e.friction_before,(uint32_t)e.friction_after,e.negotiation_before,e.negotiation_after};for(auto v:territorial)if(!word(v))return 22;
  const auto&dr=*plan.diplomacy_runtime();if(!word(dr.negotiation_faction)||!word(dr.negotiation_other)||!word(dr.negotiation_accumulator))return 22;
  for(size_t j=0;j<8;++j){for(auto v:f.faction(j)->diplo_gifts)if(!word((uint32_t)v))return 22;for(auto v:f.faction(j)->diplo_friction)if(!word((uint32_t)v))return 22;}

  const auto saved=*s;if(stage_test(w,units,tech,basic,a,b,facts,plan,1)!=PromotedAftermathError::out_of_memory||memcmp(&saved,plan.summary(),sizeof saved))return 25;
  if(!middle_guard){const size_t budget=sizeof(TerritorialBattleAftermathPlan)+f.memory_bytes();LandEngagementPlan first;
   if(stage_land_engagement(w,units,tech,basic,a,b,facts.combat,first,budget-(sizeof(TerritorialBattleAftermathPlan)-sizeof(LandEngagementPlan)))!=EngagementError::ok)return 38;
   const auto fp=fingerprint(f);const auto savedterritorial=*plan.territorial_summary();const auto saveddiplomacy=*plan.diplomacy_runtime();
   if(stage_test(w,units,tech,basic,a,b,facts,plan,budget)!=PromotedAftermathError::out_of_memory||memcmp(&saved,plan.summary(),sizeof saved)||memcmp(&savedterritorial,plan.territorial_summary(),sizeof savedterritorial)||memcmp(&saveddiplomacy,plan.diplomacy_runtime(),sizeof saveddiplomacy)||fingerprint(*plan.frame())!=fp)return 39;
   middle_guard=1;
  }
 }
 if(fingerprint(w)!=before)return 26;
}
if(accepted){const auto saved=*plan.summary();const auto*p=plan.frame();const auto staged=fingerprint(*p);const void*spots[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),w.game().bunglist.data,&units,&tech,&basic,&last};
 for(auto spot:spots)if(spot){auto*out=reinterpret_cast<TerritorialBattleAftermathPlan*>(((uintptr_t)spot+alignof(TerritorialBattleAftermathPlan)-1)&~(uintptr_t)(alignof(TerritorialBattleAftermathPlan)-1));if(stage_test(w,units,tech,basic,last_a,last_b,last,*out)!=PromotedAftermathError::bad_state)return 27;}
 if(stage_test(*p,units,tech,basic,last_a,last_b,last,plan)!=PromotedAftermathError::bad_state||memcmp(&saved,plan.summary(),sizeof saved)||fingerprint(*p)!=staged)return 28;
 for(unsigned j=0;j<3;++j){auto f=last;if(j==0)f.caller_arguments=EngagementObservation::missing;if(j==1)f.combat.multiplayer_active=true;if(j==2)f.removal.logging_enabled=true;const auto e=stage_test(w,units,tech,basic,last_a,last_b,f,plan);if(e!=(j?PromotedAftermathError::unsupported:PromotedAftermathError::bad_state)||plan.frame()!=p||memcmp(&saved,plan.summary(),sizeof saved)||fingerprint(*p)!=staged)return 29;}
 const auto savedterritory=*plan.territorial_summary();const auto saveddiplomacy=*plan.diplomacy_runtime();const auto savedbook=*plan.bookkeeping();const auto savedrun=*plan.runtime();const auto savedpools=*plan.accumulators();alignas(TerritorialBattleAftermathPlan) const auto direct=runfacts(w,last_a,last);alignas(TerritorialBattleAftermathPlan) const auto territoryrun=diplomacyfacts(w,last_a,last_b,last);
 auto check=[&](PromotedAftermathError actual,PromotedAftermathError wanted){if(actual!=wanted||plan.frame()!=p||memcmp(&saved,plan.summary(),sizeof saved)||memcmp(&savedterritory,plan.territorial_summary(),sizeof savedterritory)||memcmp(&saveddiplomacy,plan.diplomacy_runtime(),sizeof saveddiplomacy)||memcmp(&savedbook,plan.bookkeeping(),sizeof savedbook)||memcmp(&savedrun,plan.runtime(),sizeof savedrun)||memcmp(&savedpools,plan.accumulators(),sizeof savedpools)||fingerprint(*p)!=staged||fingerprint(w)!=before)return false;++runtime_guards;return true;};
 for(unsigned j=0;j<5;++j){auto r=direct;auto wanted=PromotedAftermathError::unsupported;if(j==0){r.observation=BookkeepingObservation::absent;wanted=PromotedAftermathError::bad_state;}if(j==1)r.edition=r.edition==BookkeepingEdition::smac?BookkeepingEdition::smax:BookkeepingEdition::smac;if(j==2)r.multiplayer_active=1;if(j==3)r.pbem_active=1;if(j==4){r.current_player=8;wanted=PromotedAftermathError::bad_state;}if(!check(stage_territorial_aftermath(w,units,tech,basic,last_a,last_b,last,r,territoryrun,plan),wanted))return 33;}
 for(unsigned j=0;j<2;++j){auto f=last;if(j)f.pools.killed.value=1;else f.pools.killed.present=false;if(!check(stage_territorial_aftermath(w,units,tech,basic,last_a,last_b,f,direct,territoryrun,plan),PromotedAftermathError::unsupported))return 34;}
 if(!check(stage_territorial_aftermath(w,units,tech,basic,last_a,last_b,last,*reinterpret_cast<const BattleAftermathRuntime*>(plan.bookkeeping()),territoryrun,plan),PromotedAftermathError::bad_state))return 35;
 for(unsigned j=0;j<6;++j){auto r=territoryrun;auto wanted=PromotedAftermathError::bad_state;if(j==0)r.observation=TerritoryObservation::missing;if(j==1){r.edition=r.edition==Variant::base?Variant::alien_crossfire:Variant::base;wanted=PromotedAftermathError::unsupported;}if(j==2){r.multiplayer_active=1;wanted=PromotedAftermathError::unsupported;}if(j==3){r.pbem_active=1;wanted=PromotedAftermathError::unsupported;}if(j==4)r.multiplayer_active=2;if(j==5)r.pbem_active=2;if(!check(stage_territorial_aftermath(w,units,tech,basic,last_a,last_b,last,direct,r,plan),wanted))return 37;}
 const void*poisoned[]={&units,&tech,&basic,&last,&direct,&territoryrun};const size_t sizes[]={sizeof units,sizeof tech,sizeof basic,sizeof last,sizeof direct,sizeof territoryrun};
 for(unsigned j=0;j<6;++j){const auto snapshot=hashbytes(2166136261u,poisoned[j],sizes[j]);
#ifdef __SANITIZE_ADDRESS__
  __asan_poison_memory_region(poisoned[j],sizes[j]);
#endif
  const auto e=stage_territorial_aftermath(w,units,tech,basic,last_a,last_b,last,direct,territoryrun,alias<TerritorialBattleAftermathPlan>((uintptr_t)poisoned[j]));
#ifdef __SANITIZE_ADDRESS__
  __asan_unpoison_memory_region(poisoned[j],sizes[j]);
#endif
  if(e!=PromotedAftermathError::bad_state||hashbytes(2166136261u,poisoned[j],sizes[j])!=snapshot||fingerprint(*p)!=staged||memcmp(&saved,plan.summary(),sizeof saved))return 36;
 }
 w.clear();memset(data,0,n);free(data);data=nullptr;memset(text,0,nt);free(text);text=nullptr;memset(&units,0,sizeof units);memset(&tech,0,sizeof tech);memset(&basic,0,sizeof basic);if(fingerprint(*p)!=staged)return 30;
}fprintf(stderr,"COMPOSITION_GUARD %u\nRUNTIME_GUARDS %u\n",middle_guard,runtime_guards);free(data);free(text);return 0;}
