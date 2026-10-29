/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only binding harness. */
#include "world_repair_vehicle.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t* read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64L*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)z;return p;}
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sint(uint32_t v){int32_t n;memcpy(&n,&v,4);return n;}
static bool put(uint32_t n,unsigned size=4){uint8_t b[4];for(unsigned i=0;i<size;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,size,stdout)==size;}
static alpha::Rules tech,before_tech;static alpha::UnitDefinitions units,before_units;
int main(int argc,char**argv){
 using namespace alpha;if(argc!=5 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 Variant profile=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n;auto*text=read(argv[2],n);if(!text)return 20;
 if(read_technologies(text,n,tech)!=Error::ok||read_unit_definitions(text,n,tech,profile,units)!=Error::ok)return 20;
 free(text);memcpy(&before_tech,&tech,sizeof tech);memcpy(&before_units,&units,sizeof units);auto*save=read(argv[3],n);if(!save)return 20;
 OriginalWorld world,baseline;if(world.load(save,n,123)!=WorldLoadError::ok||baseline.load(save,n,123)!=WorldLoadError::ok)return 20;
 FILE*q=fopen(argv[4],"rb");if(!q)return 20;uint8_t query[12];unsigned queries=0;
 for(;;){const size_t count=fread(query,1,12,q);if(!count){if(ferror(q))return 20;break;}if(count!=12 || ++queries>200000)return 20;
  RepairVehicleResult result;memset(&result,90,sizeof result);const auto error=world_repair_vehicle(world,units,tech,profile,sint(get(query)),sint(get(query+4)),get(query+8),result);
  if(!put((uint32_t)error))return 20;
  if(error!=LogicError::ok){const auto*p=(const uint8_t*)&result;for(size_t i=0;i<sizeof result;++i)if(p[i]!=90)return 21;for(unsigned i=0;i<72;++i)if(!put(90,1))return 20;continue;}
#define WIRE_SCALAR(type,name,offset) if(!put((uint32_t)result.vehicle.name,sizeof(type)))return 20;
#define WIRE_ARRAY(type,name,count,offset) for(auto v:result.vehicle.name)if(!put((uint32_t)v,sizeof(type)))return 20;
#define WIRE_STRING(name,count,offset)
#include "original_vehicle_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
  if(!put(result.processed) || !put(result.healing_attempted) || !put(result.draw_requested) || !put(result.healing) || !put(result.damage_floor))return 20;
 }
 fclose(q);unsigned guards=0;RepairVehicleResult out,before;memset(&out,90,sizeof out);memcpy(&before,&out,sizeof out);
 auto bad=[&](const OriginalWorld&w,Variant p,int f,int l,size_t id){++guards;return world_repair_vehicle(w,units,tech,p,f,l,id,out)==LogicError::bad_state&&!memcmp(&out,&before,sizeof out);};
 OriginalWorld empty;
 if(!bad(empty,profile,1,1,0)||!bad(world,profile,-1,1,0)||!bad(world,profile,8,1,0)||!bad(world,profile,1,-1,0)||!bad(world,profile,1,8,0)||!bad(world,(Variant)99,1,1,0)||!bad(world,profile==Variant::base?Variant::alien_crossfire:Variant::base,1,1,0)||!bad(world,profile,1,1,world.game().vehicle_count))return 21;
 --tech.count;if(!bad(world,profile,1,1,0))return 21;++tech.count;
 --units.weapon_count;if(!bad(world,profile,1,1,0))return 21;++units.weapon_count;
 --units.ability_count;if(!bad(world,profile,1,1,0))return 21;++units.ability_count;
 const void*aliases[]={&world,&units,&tech,world.original_bytes(),world.current_monuments().data,world.tile(0),world.vehicle(0),world.base(0),world.prototype(0),world.faction(0),world.faction_definition(0)};
 for(const void*at:aliases)if(at){++guards;auto&r=*reinterpret_cast<RepairVehicleResult*>(const_cast<void*>(at));if(world_repair_vehicle(world,units,tech,profile,1,1,0,r)!=LogicError::bad_state)return 21;}
 if(!put(guards)||!alpha_test::same_world(world,baseline)||memcmp(world.current_monuments().data,baseline.current_monuments().data,world.current_monuments().size)||memcmp(world.original_bytes(),save,n)||memcmp(&tech,&before_tech,sizeof tech)||memcmp(&units,&before_units,sizeof units)||world.runtime_seed()!=123)return 21;
 free(save);return 0;
}
