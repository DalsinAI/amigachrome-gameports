/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_battle_bookkeeping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static bool rd(FILE*f,uint32_t&v){v=0;for(unsigned j=0;j<4;++j){int b=fgetc(f);if(b==EOF)return false;v|=(uint32_t)b<<(8*j);}return true;}
static void write32(uint32_t v){for(unsigned j=0;j<4;++j){fputc((int)(v&255u),stdout);v>>=8;}}
static void wr(uint8_t*p,uint32_t v){for(unsigned j=0;j<4;++j){p[j]=(uint8_t)v;v>>=8;}}
static int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
static uint32_t hashbytes(uint32_t h,const void*p,size_t n){auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hashbytes(h,w.original_bytes(),w.original_size());h=hashbytes(h,w.current_monuments().data,w.current_monuments().size);h=hashbytes(h,w.game().bunglist.data,w.game().bunglist.size);h=hashbytes(h,&w.game().preferences,sizeof w.game().preferences);h=hashbytes(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hashbytes(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hashbytes(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hashbytes(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hashbytes(h,w.faction(0),8*sizeof(OriginalFactionState));return hashbytes(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||a.current_monuments().data==b.current_monuments().data||a.game().bunglist.data==b.game().bunglist.data||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;auto pref=a.game().preferences;pref.VehCount=(int32_t)b.game().vehicle_count;if(memcmp(&pref,&b.game().preferences,sizeof pref))return false;
for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;
for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;
for(size_t i=0;i<8;++i)if(memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;
#define SPAN(name) if(a.game().name.size!=b.game().name.size||(a.game().name.size&&memcmp(a.game().name.data,b.game().name.data,a.game().name.size)))return false;
SPAN(factions) SPAN(faction_definitions) SPAN(rankings) SPAN(faction_turn_might) SPAN(replay) SPAN(continents) SPAN(prototypes) SPAN(vehicles) SPAN(bases) SPAN(scenario) SPAN(scenario_name) SPAN(objectives) SPAN(monuments) SPAN(tectonic)
#undef SPAN
return true;}
static void writestate(const BattleBookkeepingState&s){for(auto v:s.wins)write32(v);for(auto v:s.losses)write32(v);for(auto&r:s.combat_total)for(auto v:r)write32(v);for(auto&r:s.combat_count)for(auto v:r)write32(v);for(auto&r:s.enemy_destroyed){write32(r.year);write32(r.opponent);write32(r.leader);write32(r.awarded);write32(r.rank);write32(r.focus);}}

static void emit(const WorldBattleBookkeepingPlan&p){const auto&r=*p.bookkeeping();writestate(r.before_cleanup);writestate(r.after_cleanup);write32(r.winner);write32(r.loser);write32(r.monument_changed);fwrite(p.frame()->current_monuments().data,1,0x27a0,stdout);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t worlds;if(!rd(f,worlds))return 20;
const uint32_t seeds[]={0,1,19,0x7ffffffeu,0x7fffffffu,0x80000000u,0xfffffffeu,0xffffffffu};
for(uint32_t world=0;world<worlds;++world){uint32_t len,rows;if(!rd(f,len)||!rd(f,rows)||len>64u*1024u*1024u)return 20;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 20;
for(uint32_t row=0;row<rows;++row){uint32_t at,de,kind,var;if(!rd(f,at)||!rd(f,de)||!rd(f,kind)||!rd(f,var)||var>=8)return 20;OriginalWorld w;if(w.load(data,len,0xabc123)!=WorldLoadError::ok)return 21;
// Deliberately changed current records, raw baseline still immutable.
if(w.vehicle(0))const_cast<OriginalVehicle*>(w.vehicle(0))->damage_taken^=0x5a;
const_cast<OriginalMapTile*>(w.tile(0))->contour^=1;if(w.base(0))const_cast<OriginalBase*>(w.base(0))->state_flags^=0x400;
const_cast<OriginalPrototype*>(w.prototype(511))->name[0]^=1;const_cast<OriginalFactionDefinition*>(w.faction_definition(0))->filename[0]^=1;
alignas(WorldBookkeepingFacts) uint8_t history[4096];for(size_t j=0;j<4096;++j)history[j]=(uint8_t)(j^var);const_cast<OriginalGameView&>(w.game()).bunglist={history,4096};
WorldBookkeepingFacts facts{BookkeepingObservation::supplied,w.game().data_version>=11?BookkeepingEdition::smax:BookkeepingEdition::smac,(int32_t)at,(int32_t)de,(BookkeepingOutcome)kind,-1,0,0,seeds[(var+2)%8],{},{} };
for(size_t i=0;i<8;++i){facts.wins[i]=seeds[(var+i)%8];facts.losses[i]=seeds[(var+i+3)%8];auto*fs=const_cast<OriginalFactionState*>(w.faction(i));fs->energy_credits^=0x234;
for(size_t j=0;j<8;++j){fs->diplo_combat_total[j]=signed32(seeds[var]+(uint32_t)(i*71+j*13));fs->diplo_combat_count[j]=signed32(seeds[(var+1)%8]+(uint32_t)(i*17+j*31));}
auto*mon=const_cast<uint8_t*>(w.current_monuments().data)+i*0x4f4;wr(mon+0x150,0xa1a1a1a1);wr(mon+0x158,0xb2b2b2b2);wr(mon+0x15c,0xc3c3c3c3);wr(mon+0x160,((var*37u)>>i)&1u);wr(mon+0x164,(var+i)%4?1u:0u);wr(mon+0x498,0xd4d4d4d4);
}
const_cast<uint8_t*>(w.current_monuments().data)[7]^=0xa5;
WorldBattleBookkeepingPlan plan;const auto before=fingerprint(w);const auto originalFacts=facts;
if(stage_world_battle_bookkeeping(w,facts,plan)!=WorldBookkeepingError::ok||fingerprint(w)!=before||memcmp(&facts,&originalFacts,sizeof facts)||!collateral(w,*plan.frame()))return 30;
// Every other decoded faction byte stays exact, including prior casualty counters.
for(size_t i=0;i<8;++i){const auto*original=w.faction(i);const auto*actual=plan.frame()->faction(i);if(!original||!actual)return 31;auto saved=*actual;for(size_t j=0;j<8;++j){saved.diplo_combat_total[j]=original->diplo_combat_total[j];saved.diplo_combat_count[j]=original->diplo_combat_count[j];}if(memcmp(&saved,original,sizeof saved))return 31;}
if(plan.frame()->game().vehicle_count!=w.game().vehicle_count||(w.game().vehicle_count&&memcmp(plan.frame()->vehicle(0),w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)))||memcmp(plan.frame()->tile(0),w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)))return 32;
const auto*p=plan.frame();const auto staged=fingerprint(*p);const auto saved=*plan.bookkeeping();unsigned guards=0;
auto guard=[&](WorldBookkeepingError e,WorldBookkeepingError wanted){if(e!=wanted||plan.frame()!=p||fingerprint(*p)!=staged||memcmp(plan.bookkeeping(),&saved,sizeof saved)||fingerprint(w)!=before||memcmp(&facts,&originalFacts,sizeof facts))return false;++guards;return true;};
if(!guard(stage_world_battle_bookkeeping(w,facts,plan,1),WorldBookkeepingError::out_of_memory))return 33;
{auto x=facts;x.observation=BookkeepingObservation::absent;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::bad_state))return 34;}
for(unsigned j=0;j<2;++j){auto x=facts;if(j)x.pbem_active=1;else x.multiplayer_active=1;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::unsupported))return 35;}
{auto x=facts;x.attacker_faction=8;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::bad_state))return 36;}
{auto x=facts;x.defender_faction=x.attacker_faction;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::bad_state))return 37;}
{OriginalWorld empty;if(!guard(stage_world_battle_bookkeeping(empty,facts,plan),WorldBookkeepingError::bad_state))return 38;}
if(saved.monument_changed){auto x=facts;x.current_player=(int32_t)saved.winner;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::monument_ui))return 39;}
if(!guard(stage_world_battle_bookkeeping(*p,facts,plan),WorldBookkeepingError::bad_state))return 40;
if(!guard(stage_world_battle_bookkeeping(w,*reinterpret_cast<const WorldBookkeepingFacts*>(plan.bookkeeping()),plan),WorldBookkeepingError::bad_state))return 41;
{auto x=facts;x.edition=facts.edition==BookkeepingEdition::smac?BookkeepingEdition::smax:BookkeepingEdition::smac;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::unsupported))return 46;}
{auto x=facts;x.edition=(BookkeepingEdition)2;if(!guard(stage_world_battle_bookkeeping(w,x,plan),WorldBookkeepingError::bad_state))return 47;}
// Numeric identity/current ownership continuation, no claim of a second legal battle.
auto continuation=facts;continuation.outcome=kind?BookkeepingOutcome::attacker_lost:BookkeepingOutcome::defender_lost;for(size_t i=0;i<8;++i){continuation.wins[i]=saved.after_cleanup.wins[i];continuation.losses[i]=saved.after_cleanup.losses[i];}
WorldBattleBookkeepingPlan next;if(stage_world_battle_bookkeeping(*p,continuation,next)!=WorldBookkeepingError::ok)return 42;
const auto continued=fingerprint(*next.frame());
if(stage_world_battle_bookkeeping(w,facts,plan)!=WorldBookkeepingError::ok||fingerprint(*plan.frame())!=staged||fingerprint(*next.frame())!=continued)return 43;
w.clear();memset(history,0,4096);if(fingerprint(*plan.frame())!=staged||fingerprint(*next.frame())!=continued)return 44;
write32(guards);emit(plan);emit(next);
}
memset(data,0,len);free(data);
}fclose(f);return ferror(stdout)?45:0;}
