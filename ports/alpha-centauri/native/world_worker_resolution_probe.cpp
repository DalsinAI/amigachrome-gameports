/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_worker_resolution.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
using namespace alpha;
alignas(WorkerResolutionResult) static Rules technologies;
alignas(WorkerResolutionResult) static UnitDefinitions units;
alignas(WorkerResolutionResult) static BasicRules basic;
alignas(WorkerResolutionResult) static WorldRules world_rules;
alignas(WorkerResolutionResult) static ResourceInfo resources;
alignas(WorkerResolutionResult) static UpkeepCitizenDefinitions citizens;
alignas(WorkerResolutionResult) static OriginalWorld world;
struct Range {const void*p;size_t n;};
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long z=ftell(f);rewind(f);if(z<=0||z>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}n=(size_t)z;const bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
static bool number(const char*s,int32_t&out){char*end=nullptr;errno=0;const long value=strtol(s,&end,10);if(errno||!end||end==s||*end||value<(-2147483647L-1)||value>2147483647L)return false;out=(int32_t)value;return true;}
static void values(const char*name,const int32_t*p,size_t n){printf("%s",name);for(size_t i=0;i<n;++i)printf(" %ld",(long)p[i]);puts("");}
int main(int argc,char**argv){
 if(argc!=6&&argc!=7)return 20;
 size_t n=0;auto*p=read(argv[1],n);if(!p)return 20;const bool parsed=read_technologies(p,n,technologies)==Error::ok&&read_unit_definitions(p,n,technologies,Variant::base,units)==Error::ok&&read_basic_rules(p,n,technologies,Variant::base,basic,world_rules)==Error::ok&&read_resource_info(p,n,resources)==Error::ok&&read_citizens(p,n,technologies,citizens)==Error::ok;memset(p,0xff,n);free(p);if(!parsed)return 20;
 int32_t base,commerce,grid;if(!number(argv[3],base)||base<0||!number(argv[4],commerce)||!number(argv[5],grid))return 20;
 WorkerResolutionStage stage{{1,false,false},commerce,grid};if(argc==7){if(!strcmp(argv[6],"inspection"))stage.worker.upkeep_state=0;else if(!strcmp(argv[6],"editor"))stage.worker.editor_active=true;else if(!strcmp(argv[6],"control"))stage.worker.control_upkeep=true;else return 20;}
 WorkerResolutionResult result,sentinel;memset(&result,0x5a,sizeof result);memcpy(&sentinel,&result,sizeof result);
 auto call=[&](size_t id,WorkerResolutionResult&out){return world_worker_resolution(world,technologies,units,basic,world_rules,resources,citizens,id,stage,out);};
 if(call(0,result)!=StableIntakeError::bad_state||memcmp(&result,&sentinel,sizeof result))return 21;
 p=read(argv[2],n);if(!p)return 20;const auto loaded=world.load(p,n,0xabcde123);memset(p,0xff,n);free(p);if(loaded!=WorldLoadError::ok)return 20;
 const auto&g=world.game();const Range ranges[]={{&world,sizeof world},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&technologies,sizeof technologies},{&units,sizeof units},{&basic,sizeof basic},{&world_rules,sizeof world_rules},{&resources,sizeof resources},{&citizens,sizeof citizens},{&stage,sizeof stage}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}auto*before=(uint8_t*)malloc(total);if(!before)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(before+at,r.p,r.n);at+=r.n;}
 for(const auto&r:ranges){if(!r.n)continue;const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets)if(call((size_t)base,*reinterpret_cast<WorkerResolutionResult*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset))!=StableIntakeError::bad_buffer)return 21;}
 const auto error=call((size_t)base,result);if(error!=StableIntakeError::ok&&memcmp(&result,&sentinel,sizeof result))return 21;printf("BASE %u\n",(unsigned)error);
 if(error==StableIntakeError::ok){const auto&r=result;
  printf("ASSIGN %ld %ld %lu %lu %lu %lu %lu %u\n",(long)r.assignment.population,(long)r.assignment.specialist_count,(unsigned long)r.assignment.worked_tiles,(unsigned long)r.assignment.specialist_types[0],(unsigned long)r.assignment.specialist_types[1],(unsigned long)r.specialist_adjust,(unsigned long)r.trimmed_workers,(unsigned)r.requires_riot_event);
  const auto&i=r.intake;printf("INTAKE");for(int32_t v:i.satellites)printf(" %ld",(long)v);for(int32_t v:i.tiles)printf(" %ld",(long)v);for(int32_t v:i.intake)printf(" %ld",(long)v);printf(" %lu %lu %ld %ld %ld",(unsigned long)i.worked_tiles,(unsigned long)i.state_flags,(long)i.raw_eco_damage,(long)i.terraform_reduction_after,(long)i.base_energy_after);for(const auto&tile:i.tile_yields)for(int32_t v:tile)printf(" %ld",(long)v);puts("");
  const auto&m=r.mineral.minerals;const int32_t minerals[]={m.intake,m.consumption,m.surplus,m.surplus_final,m.eco_damage};values("MINERAL",minerals,5);
  const auto&f=r.nutrient;const int32_t food[]={f.growth,f.intake,f.consumption,f.surplus,f.stored,f.faction_surplus};values("NUTRIENT",food,6);
  const auto&e=r.energy;printf("ENERGY %ld %ld %u %ld %ld %ld %ld",(long)e.headquarters_distance,(long)e.pending_efficiency,(unsigned)e.physical_creche,(long)e.accounting.intake,(long)e.accounting.consumption,(long)e.accounting.inefficiency,(long)e.accounting.surplus);for(int32_t v:e.accounting.social_efficiency_after)printf(" %ld",(long)v);puts("");
  const auto&a=r.allocation.allocation;printf("ALLOCATION %ld %ld %ld %ld %ld %ld %ld %ld %lu %lu %ld %ld %ld\n",(long)a.distributed_energy,(long)a.economy,(long)a.psych,(long)a.labs,(long)a.unknown_total,(long)a.coefficient_economy,(long)a.coefficient_psych,(long)a.coefficient_labs,(unsigned long)a.specialist_types[0],(unsigned long)a.specialist_types[1],(long)a.specialist_economy,(long)a.specialist_psych,(long)a.specialist_labs);
  const auto&d=r.psych.psych;printf("PSYCH %ld %ld %ld %ld %ld %ld",(long)d.talents,(long)d.drones,(long)d.superdrones,(long)d.efficiency_drones,(long)d.capture_drones,(long)d.police_total);for(const auto&row:d.rows)for(int32_t v:row)printf(" %ld",(long)v);printf(" %u %u\n",(unsigned)d.can_riot,(unsigned)d.will_riot);
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(before+at,r.p,r.n))return 21;at+=r.n;}free(before);return world.runtime_seed()==0xabcde123?0:21;
}
