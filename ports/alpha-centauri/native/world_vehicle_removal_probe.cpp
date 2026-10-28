/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_vehicle_removal.h"
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
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t cases;if(!rd(f,cases))return 20;
for(uint32_t c=0;c<cases;++c){uint32_t len,id,var;if(!rd(f,len)||!rd(f,id)||!rd(f,var)||len>64*1024*1024)return 20;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 20;OriginalWorld w;if(w.load(data,len,0xabc123)!=WorldLoadError::ok)return 21;auto&g=const_cast<OriginalGameView&>(w.game());const auto n=g.vehicle_count,nt=g.map.info.tile_count;auto*vs=const_cast<OriginalVehicle*>(w.vehicle(0));auto*ts=const_cast<OriginalMapTile*>(w.tile(0));auto*fs=const_cast<OriginalFactionState*>(w.faction(0));
// CURRENT decoded state deliberately differs from the original immutable bytes.
vs[(id+1)%n].damage_taken^=0x5a;vs[(id+1)%n].state^=0x400u;ts[0].contour^=1;fs[0].energy_credits^=0x234;const_cast<OriginalPrototype*>(w.prototype(511))->name[0]^=1;const_cast<OriginalFactionDefinition*>(w.faction_definition(0))->filename[0]^=1;if(w.base(0))const_cast<OriginalBase*>(w.base(0))->state_flags^=0x400;
const_cast<uint8_t*>(w.current_monuments().data)[7]^=0xa5;uint8_t history[4096];for(size_t j=0;j<4096;++j)history[j]=(uint8_t)(j^id);g.bunglist={history,4096};
int32_t refs[]={-1,0,(int32_t)id,(int32_t)n-1,(int32_t)n,(int32_t)n+1,1,id?(int32_t)id-1:0};RemovalRuntime run{RemovalObservation::supplied,false,false,false,{0,0,0,0},refs[(var+4)%8],0,0,0};for(unsigned j=0;j<4;++j)run.cursor[j]=refs[(var+j)%8];const auto inputRuntime=run;const auto before=fingerprint(w);const auto removed=*w.vehicle(id);
VehicleRemovalPlan plan;if(stage_vehicle_removal(w,id,run,plan)!=RemovalError::ok||!plan.ready()||fingerprint(w)!=before||memcmp(&run,&inputRuntime,sizeof run))return 30;const auto*p=plan.frame();if(p->game().vehicle_count!=n-1||p->game().preferences.VehCount!=(int32_t)n-1||p->vehicle(n-1)||!collateral(w,*p)||memcmp(plan.removed(),&removed,sizeof removed))return 31;
static OriginalFactionState expected[8];for(size_t j=0;j<8;++j)memcpy(&expected[j],w.faction(j),sizeof expected[j]);auto&ef=expected[removed.faction_id];--ef.units_active[(size_t)removed.unit_id];if((uint8_t)w.prototype((size_t)removed.unit_id)->plan<8){uint32_t bits;memcpy(&bits,&ef.total_combat_units,4);--bits;memcpy(&ef.total_combat_units,&bits,4);}for(size_t j=0;j<8;++j)if(memcmp(&expected[j],p->faction(j),sizeof expected[j]))return 32;
const auto staged=fingerprint(*p);const auto savedrun=*plan.runtime();unsigned guards=0;auto guard=[&](RemovalError actual,RemovalError want){if(actual!=want||!plan.ready()||plan.frame()!=p||fingerprint(*p)!=staged||memcmp(plan.runtime(),&savedrun,sizeof savedrun)||fingerprint(w)!=before||memcmp(&run,&inputRuntime,sizeof run))return false;++guards;return true;};
if(!guard(stage_vehicle_removal(w,n,run,plan),RemovalError::bad_state)||!guard(stage_vehicle_removal(w,id,run,plan,1),RemovalError::out_of_memory))return 33;
{OriginalWorld empty;if(!guard(stage_vehicle_removal(empty,0,run,plan),RemovalError::bad_state))return 34;}
{auto r=run;r.observation=RemovalObservation::missing;if(!guard(stage_vehicle_removal(w,id,r,plan),RemovalError::bad_state))return 35;}
for(unsigned j=0;j<3;++j){auto r=run;if(j==0)r.multiplayer_active=true;if(j==1)r.logging_enabled=true;if(j==2)r.base_window_visible=true;if(!guard(stage_vehicle_removal(w,id,r,plan),RemovalError::unsupported))return 36;}
// Reject all output/input aliases before dereferencing their unrelated types.
const void*spots[]={&w,w.original_bytes(),w.current_monuments().data,w.tile(0),w.vehicle(0),w.base(0),w.prototype(0),w.faction(0),w.faction_definition(0),history};
for(auto spot:spots)if(spot&&!guard(stage_vehicle_removal(w,id,run,*reinterpret_cast<VehicleRemovalPlan*>(const_cast<void*>(spot))),RemovalError::bad_state))return 37;
if(!guard(stage_vehicle_removal(w,id,*plan.runtime(),plan),RemovalError::bad_state))return 38;
if(!guard(stage_vehicle_removal(w,id,*reinterpret_cast<const RemovalRuntime*>(history),plan),RemovalError::bad_state))return 39;
if(!guard(stage_vehicle_removal(*p,0,run,plan),RemovalError::bad_state))return 40;
if(guards!=20)return 41;
// Continue from CURRENT staged records, then replace a plan, checking ownership.
if(n>1){VehicleRemovalPlan next;auto runtime=*plan.runtime();if(stage_vehicle_removal(*p,0,runtime,next)!=RemovalError::ok||next.frame()->game().vehicle_count!=n-2||next.frame()->game().preferences.VehCount!=(int32_t)n-2||next.frame()->vehicle(n-2)||memcmp(next.frame()->original_bytes(),data,len))return 42;}
if(stage_vehicle_removal(w,id,run,plan)!=RemovalError::ok||fingerprint(*plan.frame())!=staged)return 43;
p=plan.frame();
w.clear();memset(history,0,4096);memset(data,0,len);free(data);if(fingerprint(*p)!=staged)return 44;
if(!word(0)||!word((uint32_t)p->game().vehicle_count)||!word(guards))return 22;
for(size_t j=0;j<p->game().vehicle_count;++j)if(!vehicle(*p->vehicle(j)))return 22;
for(size_t j=0;j<nt;++j)if(!tile(*p->tile(j)))return 22;
for(size_t j=0;j<8;++j)if(fwrite(p->faction(j)->units_active,1,512,stdout)!=512||!word((uint32_t)p->faction(j)->total_combat_units))return 22;
for(auto x:plan.runtime()->cursor)if(!word((uint32_t)x))return 22;
const auto&r=*plan.runtime();if(!word((uint32_t)r.iteration_cursor)||!word((uint32_t)r.lift_vehicle)||!word((uint32_t)r.lift_x)||!word((uint32_t)r.lift_y)||!vehicle(*plan.removed()))return 22;
}if(fgetc(f)!=EOF)return 23;fclose(f);return 0;}
