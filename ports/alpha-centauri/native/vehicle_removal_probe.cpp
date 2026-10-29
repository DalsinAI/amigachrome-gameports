/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "vehicle_removal.h"
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
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t cases;if(!rd(f,cases))return 20;
for(uint32_t c=0;c<cases;++c){uint32_t len,id,var;if(!rd(f,len)||!rd(f,id)||!rd(f,var)||len>64*1024*1024)return 20;auto*data=(uint8_t*)malloc(len);if(!data||fread(data,1,len,f)!=len)return 20;OriginalWorld w;if(w.load(data,len,0xabc123)!=WorldLoadError::ok)return 21;const auto n=w.game().vehicle_count,nt=w.game().map.info.tile_count;auto*vs=const_cast<OriginalVehicle*>(w.vehicle(0));auto*ts=const_cast<OriginalMapTile*>(w.tile(0));auto*fs=const_cast<OriginalFactionState*>(w.faction(0));
int32_t refs[]={-1,0,(int32_t)id,(int32_t)n-1,(int32_t)n,(int32_t)n+1,1,id?(int32_t)id-1:0};RemovalRuntime run{RemovalObservation::supplied,false,false,false,{0,0,0,0},refs[(var+4)%8],0,0,0};for(unsigned j=0;j<4;++j)run.cursor[j]=refs[(var+j)%8];
VehicleRemovalStorage s{{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,ts,nt,vs,n},w.prototype(0),512,fs,8,&run};
auto*savedv=(OriginalVehicle*)malloc(n*sizeof(*vs));auto*savedt=(OriginalMapTile*)malloc(nt*sizeof(*ts));auto*savedf=(OriginalFactionState*)malloc(8*sizeof(*fs));if(!savedv||!savedt||!savedf)return 20;memcpy(savedv,vs,n*sizeof(*vs));memcpy(savedt,ts,nt*sizeof(*ts));memcpy(savedf,fs,8*sizeof(*fs));const RemovalRuntime savedr=run;
// Every rejected call must leave all mutable supplied storage unchanged.
unsigned rejects=0;auto reject=[&](VehicleRemovalStorage trial,size_t victim,RemovalError error){auto old=run;auto result=remove_vehicle_storage(trial,victim);if(result!=error||trial.stacks.vehicle_count!=n||memcmp(vs,savedv,n*sizeof(*vs))||memcmp(ts,savedt,nt*sizeof(*ts))||memcmp(fs,savedf,8*sizeof(*fs))||memcmp(&run,&old,sizeof run))return false;++rejects;return true;};
if(!reject(s,n,RemovalError::bad_state))return 30;
for(unsigned k=0;k<3;++k){if(k==0)run.multiplayer_active=true;if(k==1)run.logging_enabled=true;if(k==2)run.base_window_visible=true;if(!reject(s,id,RemovalError::unsupported))return 31;run=savedr;}
run.observation=RemovalObservation::missing;if(!reject(s,id,RemovalError::bad_state))return 32;run=savedr;
{auto b=s;b.runtime=(RemovalRuntime*)&s;if(!reject(b,id,RemovalError::bad_state))return 33;}
{auto b=s;b.prototypes=(OriginalPrototype*)vs;if(!reject(b,id,RemovalError::bad_state))return 34;}
{auto b=s;b.factions=(OriginalFactionState*)ts;if(!reject(b,id,RemovalError::bad_state))return 35;}
{auto b=s;b.stacks.tiles=(OriginalMapTile*)vs;if(!reject(b,id,RemovalError::bad_state))return 36;}
{auto b=s;b.prototype_count=511;if(!reject(b,id,RemovalError::bad_state))return 37;}
{auto b=s;b.runtime=nullptr;if(!reject(b,id,RemovalError::bad_state))return 38;}
// Malformed topology check: account for the deliberately changed input itself.
vs[id].prev_veh_id_stack=(int16_t)n;savedv[id]=vs[id];if(!reject(s,id,RemovalError::bad_state))return 39;vs[id].prev_veh_id_stack= (int16_t)(var>=8?(id?int32_t(id)-1:-1):-1);savedv[id]=vs[id];
const auto result=remove_vehicle_storage(s,id);if(result!=RemovalError::ok||s.stacks.vehicle_count!=n-1||validate_vehicle_stacks(s.stacks)!=StackError::ok)return 40;
// Only the two original faction fields may change; raw baseline stays owned/unchanged.
const auto&removed=savedv[id];const auto*p=w.prototype((size_t)removed.unit_id);auto&sf=savedf[removed.faction_id];--sf.units_active[(size_t)removed.unit_id];if((uint8_t)p->plan<8){uint32_t bits;memcpy(&bits,&sf.total_combat_units,4);--bits;memcpy(&sf.total_combat_units,&bits,4);}if(memcmp(savedf,fs,8*sizeof(*fs))||memcmp(data,w.original_bytes(),len)||w.runtime_seed()!=0xabc123)return 41;
if(!word(0)||!word((uint32_t)s.stacks.vehicle_count)||!word(rejects))return 22;
for(size_t j=0;j<n;++j)if(!vehicle(vs[j]))return 22;
for(size_t j=0;j<nt;++j)if(!tile(ts[j]))return 22;
for(size_t j=0;j<8;++j){if(fwrite(fs[j].units_active,1,512,stdout)!=512||!word((uint32_t)fs[j].total_combat_units))return 22;}
for(auto x:run.cursor)if(!word((uint32_t)x))return 22;
if(!word((uint32_t)run.iteration_cursor)||!word((uint32_t)run.lift_vehicle)||!word((uint32_t)run.lift_x)||!word((uint32_t)run.lift_y))return 22;
free(savedv);free(savedt);free(savedf);free(data);
}if(fgetc(f)!=EOF)return 23;fclose(f);return 0;}
