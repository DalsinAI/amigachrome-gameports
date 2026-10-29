/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_casualty.h"
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
for(uint32_t c=0;c<count;++c){uint32_t len,id,var;if(!rd(f,len)||!rd(f,id)||!rd(f,var)||len>64*1024*1024)return 20;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 20;OriginalWorld w;if(w.load(data,len,0xabc123)!=WorldLoadError::ok)return 21;const auto n=w.game().vehicle_count,nt=w.game().map.info.tile_count;const_cast<OriginalVehicle*>(w.vehicle(id))->state^=0x100u;const_cast<OriginalVehicle*>(w.vehicle(id))->flags^=8u;const_cast<OriginalMapTile*>(w.tile(0))->items^=0x20u;const auto before=fingerprint(w);const auto removed=*w.vehicle(id);
RemovalRuntime run{RemovalObservation::supplied,false,false,false,{0,(int32_t)id,(int32_t)n,-1},(int32_t)id,0,0,0};
const int32_t other=n>1?(int32_t)((id+1)%n):-1;
CasualtyInputs inputs{other,(int32_t)(var%9)-1,{(var&1)!=0,2147483647},{(var&2)!=0,-2147483647},{(var&4)!=0,2147483647}};
const auto inputcopy=inputs;const auto runtimecopy=run;
CasualtyPlan plan;const auto first=stage_casualty(w,id,run,inputs,plan);if(first!=RemovalError::ok){fprintf(stderr,"case%u first%d\n",c,(int)first);return 30;}if(!plan.ready()||fingerprint(w)!=before||memcmp(&inputs,&inputcopy,sizeof inputs)||memcmp(&run,&runtimecopy,sizeof run))return 30;const auto*p=plan.frame();if(p->game().vehicle_count!=n-1||p->game().preferences.VehCount!=(int32_t)n-1||!collateral(w,*p)||memcmp(plan.removed(),&removed,sizeof removed))return 31;
// Compare every decoded faction field after masking ONLY permitted changes.
for(size_t j=0;j<8;++j){auto actual=*p->faction(j);actual.units_lost[(size_t)removed.unit_id]=w.faction(j)->units_lost[(size_t)removed.unit_id];actual.units_active[(size_t)removed.unit_id]=w.faction(j)->units_active[(size_t)removed.unit_id];actual.total_combat_units=w.faction(j)->total_combat_units;actual.unk_101=w.faction(j)->unk_101;if(memcmp(&actual,w.faction(j),sizeof actual))return 32;}
const auto staged=fingerprint(*p);const auto pools=*plan.accumulators();const auto runtime=*plan.runtime();unsigned guards=0;
auto guard=[&](RemovalError got,RemovalError want){if(got!=want||!plan.ready()||plan.frame()!=p||fingerprint(*p)!=staged||memcmp(plan.accumulators(),&pools,sizeof pools)||memcmp(plan.runtime(),&runtime,sizeof runtime)||fingerprint(w)!=before)return false;++guards;return true;};
if(!guard(stage_casualty(w,n,run,inputs,plan),RemovalError::bad_state)||!guard(stage_casualty(w,id,run,inputs,plan,1),RemovalError::out_of_memory))return 33;
{auto bad=inputs;bad.counterpart_actor=-2;if(!guard(stage_casualty(w,id,run,bad,plan),RemovalError::bad_state))return 34;bad=inputs;bad.counterpart_actor=(int32_t)n;if(!guard(stage_casualty(w,id,run,bad,plan),RemovalError::bad_state))return 34;bad=inputs;bad.faction_argument6=8;if(!guard(stage_casualty(w,id,run,bad,plan),RemovalError::bad_state))return 34;}
for(unsigned j=0;j<4;++j){auto r=run;if(j==0)r.observation=RemovalObservation::missing;if(j==1)r.multiplayer_active=true;if(j==2)r.logging_enabled=true;if(j==3)r.base_window_visible=true;if(!guard(stage_casualty(w,id,r,inputs,plan),j?RemovalError::unsupported:RemovalError::bad_state))return 35;}
const void*spots[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),w.game().bunglist.data};
for(auto spot:spots)if(spot&&!guard(stage_casualty(w,id,run,inputs,*reinterpret_cast<CasualtyPlan*>(((uintptr_t)spot+alignof(CasualtyPlan)-1)&~(uintptr_t)(alignof(CasualtyPlan)-1))),RemovalError::bad_state))return 36;
if(!guard(stage_casualty(w,id,*plan.runtime(),inputs,plan),RemovalError::bad_state)||!guard(stage_casualty(w,id,run,*plan.accumulators(),plan),RemovalError::bad_state)||!guard(stage_casualty(*p,0,run,inputs,plan),RemovalError::bad_state))return 37;
// Held artifact and interlude routes must not replace an existing staged frame.
auto*proto=const_cast<OriginalPrototype*>(w.prototype((size_t)removed.unit_id));const auto oldplan=proto->plan;proto->plan=12;const auto heldbefore=fingerprint(w);if(stage_casualty(w,id,run,inputs,plan)!=RemovalError::unsupported||fingerprint(w)!=heldbefore||fingerprint(*p)!=staged)return 38;proto->plan=oldplan;
if(other>=0){auto*v=const_cast<OriginalVehicle*>(w.vehicle(id));const auto state=v->state;auto&pref=const_cast<OriginalGamePreferences&>(w.game().preferences);const auto gamepref=pref.GamePreferences;v->state|=0x800000;pref.GamePreferences&=~0x40000;const auto snap=fingerprint(w);if(stage_casualty(w,id,run,inputs,plan)!=RemovalError::unsupported||fingerprint(w)!=snap||fingerprint(*p)!=staged)return 39;v->state=state;pref.GamePreferences=gamepref;}
if(stage_casualty(w,id,run,inputs,plan)!=RemovalError::ok||fingerprint(*plan.frame())!=staged)return 40;
p=plan.frame();w.clear();memset(data,0,len);free(data);if(fingerprint(*p)!=staged)return 41;
if(!word((uint32_t)p->game().vehicle_count)||!word(guards))return 22;
for(size_t j=0;j<p->game().vehicle_count;++j)if(!vehicle(*p->vehicle(j)))return 22;
for(size_t j=0;j<nt;++j)if(!tile(*p->tile(j)))return 22;
for(size_t j=0;j<8;++j){const auto&fs=*p->faction(j);if(fwrite(fs.units_active,1,512,stdout)!=512)return 22;for(auto v:fs.units_lost)if(!word(v,2))return 22;if(!word((uint32_t)fs.total_combat_units)||!word((uint32_t)fs.unk_101)||!word((uint32_t)fs.energy_credits))return 22;}
for(auto x:plan.runtime()->cursor)if(!word((uint32_t)x))return 22;
const auto&r=*plan.runtime();if(!word((uint32_t)r.iteration_cursor)||!word((uint32_t)r.lift_vehicle)||!word((uint32_t)r.lift_x)||!word((uint32_t)r.lift_y)||!vehicle(*plan.removed()))return 22;
const auto&i=*plan.accumulators();const CasualtyAccumulator*accs[]={&i.killed,&i.captured,&i.native_reward};for(const auto*a:accs)if(!word(a->present?1:0)||!word((uint32_t)a->value))return 22;
}if(fgetc(f)!=EOF)return 23;fclose(f);return 0;}
