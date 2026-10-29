/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_accounting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
using namespace alpha;
alignas(WorldAccountingResult) static Rules technologies;
alignas(WorldAccountingResult) static UnitDefinitions units;
alignas(WorldAccountingResult) static BasicRules basic;
alignas(WorldAccountingResult) static WorldRules world_rules;
alignas(WorldAccountingResult) static ResourceInfo resources;
alignas(WorldAccountingResult) static OriginalWorld world;
struct Range{const void*p;size_t n;};
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}n=(size_t)z;const bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
static bool number(const char*text,int32_t&value){char*end=nullptr;errno=0;long n=strtol(text,&end,10);if(errno||!end||end==text||*end||n<(-2147483647L-1)||n>2147483647L)return false;value=(int32_t)n;return true;}
static bool sentinel(const uint32_t*p,size_t n){for(size_t i=0;i<n;++i)if(p[i]!=0x5a5a5a5au)return false;return true;}
int main(int argc,char**argv){
 if(argc<6||argc>7||(strcmp(argv[1],"base")&&strcmp(argv[1],"expansion")))return 20;
 const Variant variant=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n=0;auto*p=read(argv[2],n);if(!p)return 20;
 const bool parsed=read_technologies(p,n,technologies)==Error::ok&&read_unit_definitions(p,n,technologies,variant,units)==Error::ok&&read_basic_rules(p,n,technologies,variant,basic,world_rules)==Error::ok&&read_resource_info(p,n,resources)==Error::ok;free(p);if(!parsed)return 20;
 p=read(argv[3],n);if(!p)return 20;const auto loaded=world.load(p,n,0xabcde123);free(p);if(loaded!=WorldLoadError::ok)return 20;
 WorldAccountingStage stage{{variant,false,false,{0,false},0,0},0,0};if(!number(argv[4],stage.commerce)||!number(argv[5],stage.energy_grid))return 20;
 if(argc==7){if(!strcmp(argv[6],"upkeep"))stage.intake.worker.upkeep_state=1;else if(!strcmp(argv[6],"editor"))stage.intake.worker.editor_active=true;else return 20;}
 const auto&g=world.game();const size_t count=g.vehicle_count,capacity=count+4;auto*states=(uint32_t*)malloc(capacity*sizeof(uint32_t));if(!states)return 20;
 const Range ranges[]={{&world,sizeof world},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&technologies,sizeof technologies},{&units,sizeof units},{&basic,sizeof basic},{&world_rules,sizeof world_rules},{&resources,sizeof resources},{&stage,sizeof stage}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}auto*baseline=(uint8_t*)malloc(total);if(!baseline)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(baseline+at,r.p,r.n);at+=r.n;}
 WorldAccountingResult output;memset(&output,0x5a,sizeof output);WorldAccountingResult before;memcpy(&before,&output,sizeof output);for(size_t i=0;i<capacity;++i)states[i]=0x5a5a5a5au;
 auto call=[&](size_t id,WorldAccountingResult&out,uint32_t*buffer,size_t cap){return world_base_accounting(world,technologies,units,basic,world_rules,resources,id,stage,out,buffer,cap);};
 for(const auto&r:ranges){if(!r.n)continue;const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets){auto*address=const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset;
  if(call(0,*reinterpret_cast<WorldAccountingResult*>(address),states,capacity)!=StableIntakeError::bad_buffer||!sentinel(states,capacity))return 21;
  if(call(0,output,reinterpret_cast<uint32_t*>(address),capacity)!=StableIntakeError::bad_buffer||memcmp(&output,&before,sizeof output))return 21;
 }}
 if(call(0,output,reinterpret_cast<uint32_t*>(&output),capacity)!=StableIntakeError::bad_buffer||memcmp(&output,&before,sizeof output))return 21;
 if(call(0,*reinterpret_cast<WorldAccountingResult*>(states),states,capacity)!=StableIntakeError::bad_buffer||!sentinel(states,capacity))return 21;
 if(call(0,output,states,SIZE_MAX/sizeof(uint32_t)+1)!=StableIntakeError::bad_buffer||memcmp(&output,&before,sizeof output)||!sentinel(states,capacity))return 21;
 if(count&&(call(0,output,states,count-1)!=StableIntakeError::bad_buffer||memcmp(&output,&before,sizeof output)||!sentinel(states,capacity)))return 21;
 if(count&&(call(0,output,nullptr,count)!=StableIntakeError::bad_buffer||memcmp(&output,&before,sizeof output)||!sentinel(states,capacity)))return 21;
 if(call(g.base_count,output,states,capacity)!=StableIntakeError::bad_state||memcmp(&output,&before,sizeof output)||!sentinel(states,capacity))return 21;
 for(size_t id=0;id<g.base_count;++id){memset(&output,0x5a,sizeof output);for(size_t i=0;i<capacity;++i)states[i]=0x5a5a5a5au;const auto error=call(id,output,states,capacity);
  if(error!=StableIntakeError::ok&&(memcmp(&output,&before,sizeof output)||!sentinel(states,capacity)))return 21;
  if(!sentinel(states+count,capacity-count))return 21;
  printf("BASE %lu %d\n",(unsigned long)id,(int)error);if(error!=StableIntakeError::ok)continue;
  const auto&s=output.support;const int32_t support[]={s.supported,s.paid_units,s.mineral_cost,s.pacifism_units,s.drone_units,s.free_skips,s.free_allowance,s.cost_per_paid_unit};printf("SUPPORT %lu",(unsigned long)id);for(int32_t value:support)printf(" %ld",(long)value);puts("");
  const auto&f=output.mineral.facts;const auto&m=output.mineral.minerals;const int32_t minerals[]={f.fresh_intake,f.convoy_to,f.forces_maintenance,f.convoy_from,f.raw_eco_damage,f.planet_ecology,f.satellites,f.population,f.major_atrocities,f.tectonic_detonations,f.sea_level,f.sea_level_rises,f.faction_difficulty,f.game_difficulty,f.tech_ranking,f.transcendent_techs,f.planet_pending,f.native_life,f.quantum_converter,f.robotic_assembly,f.genejack,f.nanoreplicator,f.bulk_matter,f.singularity_inductor,f.centauri_preserve,f.temple,f.pholus,f.human,f.ascending_transcendence,f.perihelion,f.space_elevator_orbital,m.intake,m.consumption,m.surplus,m.surplus_final,m.eco_damage};printf("MINERAL %lu",(unsigned long)id);for(int32_t value:minerals)printf(" %ld",(long)value);puts("");
  const auto&food=output.nutrient;const int32_t nutrients[]={food.growth,food.intake,food.consumption,food.surplus,food.stored,food.faction_surplus};printf("NUTRIENT %lu",(unsigned long)id);for(int32_t value:nutrients)printf(" %ld",(long)value);puts("");
  const auto&e=output.energy;printf("ENERGY %lu %ld %ld %d %ld %ld %ld %ld",(unsigned long)id,(long)e.headquarters_distance,(long)e.pending_efficiency,e.physical_creche,(long)e.accounting.intake,(long)e.accounting.consumption,(long)e.accounting.inefficiency,(long)e.accounting.surplus);for(int32_t value:e.accounting.social_efficiency_after)printf(" %ld",(long)value);puts("");
  printf("SHORTFALL %lu %lu",(unsigned long)id,(unsigned long)output.energy_shortfall.count);for(uint32_t value:output.energy_shortfall.cancel)printf(" %lu",(unsigned long)value);puts("");
  printf("STATES %lu %lu",(unsigned long)id,(unsigned long)count);for(size_t i=0;i<count;++i)printf(" %lu",(unsigned long)states[i]);puts("");
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(baseline+at,r.p,r.n))return 21;at+=r.n;}free(baseline);free(states);return world.runtime_seed()==0xabcde123?0:21;
}
