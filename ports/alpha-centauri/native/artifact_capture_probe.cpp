/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "artifact_capture.h"
#include "original_world.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static bool word(uint32_t n,unsigned bytes=4){for(unsigned i=0;i<bytes;++i){if(fputc((int)(n&255),stdout)==EOF)return false;n>>=8;}return true;}
static bool rd(FILE*f,uint32_t&v){v=0;for(unsigned i=0;i<4;++i){int c=fgetc(f);if(c==EOF)return false;v|=(uint32_t)c<<(i*8);}return true;}
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
static uint32_t hashbytes(uint32_t h,const void*p,size_t n){auto*b=(const uint8_t*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;h=hashbytes(h,w.original_bytes(),w.original_size());h=hashbytes(h,w.current_monuments().data,w.current_monuments().size);h=hashbytes(h,w.game().bunglist.data,w.game().bunglist.size);h=hashbytes(h,&w.game().preferences,sizeof w.game().preferences);h=hashbytes(h,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile));h=hashbytes(h,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle));h=hashbytes(h,w.base(0),w.game().base_count*sizeof(OriginalBase));h=hashbytes(h,w.prototype(0),512*sizeof(OriginalPrototype));h=hashbytes(h,w.faction(0),8*sizeof(OriginalFactionState));return hashbytes(h,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
static bool collateral(const OriginalWorld&a,const OriginalWorld&b){if(a.original_size()!=b.original_size()||a.original_bytes()==b.original_bytes()||a.runtime_seed()!=b.runtime_seed()||memcmp(a.original_bytes(),b.original_bytes(),a.original_size())||a.current_monuments().data==b.current_monuments().data||memcmp(a.current_monuments().data,b.current_monuments().data,a.current_monuments().size)||a.game().bunglist.data==b.game().bunglist.data||memcmp(a.game().bunglist.data,b.game().bunglist.data,4096)||memcmp(&a.game().map.info,&b.game().map.info,sizeof a.game().map.info))return false;auto pref=a.game().preferences;pref.VehCount=(int32_t)b.game().vehicle_count;if(memcmp(&pref,&b.game().preferences,sizeof pref))return false;
for(size_t i=0;i<512;++i)if(memcmp(a.prototype(i),b.prototype(i),sizeof(OriginalPrototype)))return false;
for(size_t i=0;i<a.game().base_count;++i)if(memcmp(a.base(i),b.base(i),sizeof(OriginalBase)))return false;
for(size_t i=0;i<8;++i)if(memcmp(a.faction_definition(i),b.faction_definition(i),sizeof(OriginalFactionDefinition)))return false;
#define SPAN(name) if(a.game().name.size!=b.game().name.size||(a.game().name.size&&memcmp(a.game().name.data,b.game().name.data,a.game().name.size)))return false;
SPAN(factions) SPAN(faction_definitions) SPAN(rankings) SPAN(faction_turn_might) SPAN(replay) SPAN(continents) SPAN(prototypes) SPAN(vehicles) SPAN(bases) SPAN(scenario) SPAN(scenario_name) SPAN(objectives) SPAN(monuments) SPAN(tectonic)
#undef SPAN
return true;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t count;if(!rd(f,count))return 20;
for(uint32_t c=0;c<count;++c){uint32_t len,id,var;if(!rd(f,len)||!rd(f,id)||!rd(f,var)||len>64*1024*1024)return 20;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 20;OriginalWorld w;if(w.load(data,len,0xabc123)!=WorldLoadError::ok)return 21;const auto n=w.game().vehicle_count,nt=w.game().map.info.tile_count;const auto recipient=(id+1)%n;auto*v=const_cast<OriginalVehicle*>(w.vehicle(id));v->state^=0x400u;v->flags^=8u;const_cast<OriginalMapTile*>(w.tile(0))->contour^=1;
const auto before=fingerprint(w);const auto actor=*v;
static UnitDefinitions units;units=UnitDefinitions{};const bool expansion=w.game().data_version>=11;units.weapon_count=expansion?26:21;units.armor_count=expansion?14:10;units.ability_count=expansion?29:24;units.chassis[0].speed=1+(int32_t)(var%4);units.chassis[1].speed=2;units.weapons[0].offense=1;
static Rules tech;tech=Rules{};tech.count=89;
const auto tableshash=hashbytes(hashbytes(2166136261u,&units,sizeof units),&tech,sizeof tech);
RemovalRuntime run{RemovalObservation::supplied,false,false,false,{0,(int32_t)id,(int32_t)n,-1},(int32_t)id,123,456,789};const auto runtimecopy=run;
CasualtyInputs in{(int32_t)recipient,(int32_t)(var%9)-1,{(var&1)!=0,2147483647},{(var&2)!=0,2147483647},{(var&4)!=0,-5}};const auto incopy=in;
ArtifactCapturePlan plan;const auto first=stage_artifact_capture(w,id,run,units,tech,1+(int32_t)(var%3),in,plan);if(first!=RemovalError::ok){fprintf(stderr,"case%u first%d\n",c,(int)first);return 30;}
if(!plan.ready()||fingerprint(w)!=before||memcmp(&run,&runtimecopy,sizeof run)||memcmp(&in,&incopy,sizeof in)||tableshash!=hashbytes(hashbytes(2166136261u,&units,sizeof units),&tech,sizeof tech))return 31;
const auto*p=plan.frame();if(p->game().vehicle_count!=n||p->game().preferences.VehCount!=(int32_t)n||!collateral(w,*p)||memcmp(plan.before(),&actor,sizeof actor))return 32;
for(size_t j=0;j<8;++j){auto actual=*p->faction(j);actual.units_lost[(size_t)actor.unit_id]=w.faction(j)->units_lost[(size_t)actor.unit_id];actual.units_active[(size_t)actor.unit_id]=w.faction(j)->units_active[(size_t)actor.unit_id];if(memcmp(&actual,w.faction(j),sizeof actual))return 33;}
const auto staged=fingerprint(*p);const auto pools=*plan.accumulators();const auto runtime=*plan.runtime();unsigned guards=0;
auto guard=[&](RemovalError got,RemovalError want){if(got!=want||!plan.ready()||plan.frame()!=p||fingerprint(*p)!=staged||memcmp(plan.accumulators(),&pools,sizeof pools)||memcmp(plan.runtime(),&runtime,sizeof runtime)||fingerprint(w)!=before)return false;++guards;return true;};
const int32_t road=1+(int32_t)(var%3);
if(!guard(stage_artifact_capture(w,n,run,units,tech,road,in,plan),RemovalError::bad_state)||!guard(stage_artifact_capture(w,id,run,units,tech,road,in,plan,1),RemovalError::out_of_memory))return 34;
const int32_t rates[]={0,101};for(const auto rate:rates)if(!guard(stage_artifact_capture(w,id,run,units,tech,rate,in,plan),RemovalError::bad_state))return 35;
const int32_t invalidothers[]={-1,(int32_t)n};for(const auto other:invalidothers){auto bad=in;bad.counterpart_actor=other;if(!guard(stage_artifact_capture(w,id,run,units,tech,road,bad,plan),RemovalError::bad_state))return 36;}
for(unsigned j=0;j<4;++j){auto r=run;if(j==0)r.observation=RemovalObservation::missing;if(j==1)r.multiplayer_active=true;if(j==2)r.logging_enabled=true;if(j==3)r.base_window_visible=true;if(!guard(stage_artifact_capture(w,id,r,units,tech,road,in,plan),j?RemovalError::unsupported:RemovalError::bad_state))return 37;}
const void*spots[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),w.game().bunglist.data};
for(auto spot:spots)if(spot&&!guard(stage_artifact_capture(w,id,run,units,tech,road,in,*reinterpret_cast<ArtifactCapturePlan*>(((uintptr_t)spot+alignof(ArtifactCapturePlan)-1)&~(uintptr_t)(alignof(ArtifactCapturePlan)-1))),RemovalError::bad_state))return 38;
if(!guard(stage_artifact_capture(w,id,*plan.runtime(),units,tech,road,in,plan),RemovalError::bad_state)||!guard(stage_artifact_capture(w,id,run,units,tech,road,*plan.accumulators(),plan),RemovalError::bad_state)||!guard(stage_artifact_capture(*p,0,run,units,tech,road,in,plan),RemovalError::bad_state))return 39;
// Holds preserve the complete input and already-owned output at each snapshot.
unsigned holds=0;auto held=[&](RemovalError e,uint32_t snapshot){if(e!=RemovalError::unsupported||fingerprint(w)!=snapshot||fingerprint(*p)!=staged||memcmp(plan.accumulators(),&pools,sizeof pools))return false;++holds;return true;};
{auto bad=in;bad.counterpart_actor=(int32_t)id;if(!held(stage_artifact_capture(w,id,run,units,tech,road,bad,plan),before))return 40;}
auto*other=const_cast<OriginalVehicle*>(w.vehicle(recipient));const auto oldfac=other->faction_id;other->faction_id=0;auto snap=fingerprint(w);if(!held(stage_artifact_capture(w,id,run,units,tech,road,in,plan),snap))return 41;other->faction_id=oldfac;
auto*proto=const_cast<OriginalPrototype*>(w.prototype((size_t)actor.unit_id));const auto originalplan=proto->plan;proto->plan=0;snap=fingerprint(w);if(!held(stage_artifact_capture(w,id,run,units,tech,road,in,plan),snap))return 42;proto->plan=originalplan;
const auto*rp=w.prototype((size_t)other->unit_id);units.chassis[rp->chassis_id].missile=1;if(!held(stage_artifact_capture(w,id,run,units,tech,road,in,plan),before))return 43;units.chassis[rp->chassis_id].missile=0;
units.chassis[0].triad=2;if(!held(stage_artifact_capture(w,id,run,units,tech,road,in,plan),before))return 44;units.chassis[0].triad=0;
const auto destination=((size_t)other->y*w.game().map.info.width+other->x)/2;auto*desttile=const_cast<OriginalMapTile*>(w.tile(destination));const auto climate=desttile->climate;desttile->climate=64;snap=fingerprint(w);if(!held(stage_artifact_capture(w,id,run,units,tech,road,in,plan),snap))return 45;desttile->climate=climate;
units.chassis[0].speed=99;if(!held(stage_artifact_capture(w,id,run,units,tech,100,in,plan),before))return 46;units.chassis[0].speed=1+(int32_t)(var%4);
if(guards!=23||holds!=7||fingerprint(w)!=before)return 47;
if(stage_artifact_capture(w,id,run,units,tech,road,in,plan)!=RemovalError::ok||fingerprint(*plan.frame())!=staged)return 48;
p=plan.frame();w.clear();memset(data,0,len);free(data);memset(&units,0,sizeof units);memset(&tech,0,sizeof tech);if(fingerprint(*p)!=staged)return 49;
if(!word(n)||!word(guards)||!word(holds))return 22;
for(size_t j=0;j<n;++j)if(!vehicle(*p->vehicle(j)))return 22;
for(size_t j=0;j<nt;++j)if(!tile(*p->tile(j)))return 22;
for(size_t j=0;j<8;++j){const auto&fs=*p->faction(j);if(fwrite(fs.units_active,1,512,stdout)!=512)return 22;for(auto x:fs.units_lost)if(!word(x,2))return 22;if(!word((uint32_t)fs.total_combat_units)||!word((uint32_t)fs.unk_101)||!word((uint32_t)fs.energy_credits))return 22;}
for(auto x:plan.runtime()->cursor)if(!word((uint32_t)x))return 22;
const auto&r=*plan.runtime();if(!word((uint32_t)r.iteration_cursor)||!word((uint32_t)r.lift_vehicle)||!word((uint32_t)r.lift_x)||!word((uint32_t)r.lift_y)||!vehicle(*plan.before()))return 22;
const auto&inp=*plan.accumulators();const CasualtyAccumulator*accs[]={&inp.killed,&inp.captured,&inp.native_reward};for(const auto*a:accs)if(!word(a->present?1:0)||!word((uint32_t)a->value))return 22;
const auto&preview=*plan.preview();if(!word((uint32_t)preview.actor)||!word((uint32_t)preview.recipient)||!word(preview.old_faction)||!word(preview.new_faction)||!word(preview.speed))return 22;
}if(fgetc(f)!=EOF)return 23;fclose(f);return 0;}
