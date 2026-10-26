/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_minerals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
using namespace alpha;
alignas(WorldMineralResult) static Rules technologies;
alignas(WorldMineralResult) static BasicRules basic;
alignas(WorldMineralResult) static WorldRules world_rules;
alignas(WorldMineralResult) static ResourceInfo resources;
alignas(WorldMineralResult) static OriginalWorld world;
struct Range{const void*p;size_t n;};
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}n=(size_t)z;bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
int main(int argc,char**argv){
 if(argc<5||argc>6||(strcmp(argv[1],"base")&&strcmp(argv[1],"expansion")))return 20;
 const Variant variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n;auto*p=read(argv[2],n);if(!p)return 20;
 const bool parsed=read_technologies(p,n,technologies)==Error::ok&&read_basic_rules(p,n,technologies,variant,basic,world_rules)==Error::ok&&read_resource_info(p,n,resources)==Error::ok;free(p);if(!parsed)return 20;
 p=read(argv[3],n);if(!p)return 20;auto loaded=world.load(p,n,0xabcde123);free(p);if(loaded!=WorldLoadError::ok)return 20;
 char*end=nullptr;errno=0;long maintenance=strtol(argv[4],&end,10);if(errno||!end||end==argv[4]||*end||maintenance<(-2147483647L-1)||maintenance>2147483647L)return 20;
 WorldMineralStage stage{{variant,false,false,{0,false},0,0},(int32_t)maintenance};
 if(argc==6){if(!strcmp(argv[5],"upkeep"))stage.intake.worker.upkeep_state=1;else if(!strcmp(argv[5],"editor"))stage.intake.worker.editor_active=true;else if(!strcmp(argv[5],"profile"))stage.intake.variant=variant==Variant::base?Variant::alien_crossfire:Variant::base;else return 20;}
 const auto&g=world.game();const Range ranges[]={{&world,sizeof world},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&technologies,sizeof technologies},{&basic,sizeof basic},{&world_rules,sizeof world_rules},{&resources,sizeof resources},{&stage,sizeof stage}};
 size_t total=0;for(const auto&r:ranges)total+=r.n;auto*baseline=(uint8_t*)malloc(total);if(!baseline)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(baseline+at,r.p,r.n);at+=r.n;}
 for(const auto&r:ranges){if(!r.n)continue;const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets){auto&out=*reinterpret_cast<WorldMineralResult*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset);if(world_base_minerals(world,technologies,basic,world_rules,resources,0,stage,out)!=StableIntakeError::bad_buffer)return 21;}}
 WorldMineralResult bad;memset(&bad,0x5a,sizeof bad);WorldMineralResult previous;memcpy(&previous,&bad,sizeof bad);if(world_base_minerals(world,technologies,basic,world_rules,resources,g.base_count,stage,bad)!=StableIntakeError::bad_state||memcmp(&bad,&previous,sizeof bad))return 21;
 for(size_t id=0;id<g.base_count;++id){WorldMineralResult result;memset(&result,0x5a,sizeof result);const auto error=world_base_minerals(world,technologies,basic,world_rules,resources,id,stage,result);if(error!=StableIntakeError::ok&&memcmp(&result,&previous,sizeof result))return 21;
  printf("RESULT %lu %d",(unsigned long)id,(int)error);
  if(error==StableIntakeError::ok){const auto&f=result.facts;const int32_t facts[]={f.fresh_intake,f.convoy_to,f.forces_maintenance,f.convoy_from,f.raw_eco_damage,f.planet_ecology,f.satellites,f.population,f.major_atrocities,f.tectonic_detonations,f.sea_level,f.sea_level_rises,f.faction_difficulty,f.game_difficulty,f.tech_ranking,f.transcendent_techs,f.planet_pending,f.native_life,f.quantum_converter,f.robotic_assembly,f.genejack,f.nanoreplicator,f.bulk_matter,f.singularity_inductor,f.centauri_preserve,f.temple,f.pholus,f.human,f.ascending_transcendence,f.perihelion,f.space_elevator_orbital,result.minerals.intake,result.minerals.consumption,result.minerals.surplus,result.minerals.surplus_final,result.minerals.eco_damage};for(int32_t value:facts)printf(" %ld",(long)value);}
  puts("");
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(baseline+at,r.p,r.n))return 21;at+=r.n;}free(baseline);return world.runtime_seed()==0xabcde123?0:21;
}
