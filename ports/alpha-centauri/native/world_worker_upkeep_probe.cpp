/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_worker_upkeep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
alignas(WorkerUpkeepCandidate) static Rules technologies;
alignas(WorkerUpkeepCandidate) static UnitDefinitions units;
alignas(WorkerUpkeepCandidate) static BasicRules basic;
alignas(WorkerUpkeepCandidate) static ResourceInfo resources;
alignas(WorkerUpkeepCandidate) static OriginalWorld world;
struct Range {const void*p;size_t n;};
static uint8_t*read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<1||z>64*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}n=(size_t)z;const bool ok=fread(p,1,n,f)==n;fclose(f);if(!ok){free(p);return nullptr;}return p;}
int main(int argc,char**argv){
 if(argc<3||argc>4)return 20;
 size_t n=0;auto*p=read(argv[1],n);if(!p)return 20;WorldRules wr{};
 const bool parsed=read_technologies(p,n,technologies)==Error::ok&&read_unit_definitions(p,n,technologies,Variant::base,units)==Error::ok&&read_basic_rules(p,n,technologies,Variant::base,basic,wr)==Error::ok&&read_resource_info(p,n,resources)==Error::ok;memset(p,0xff,n);free(p);if(!parsed)return 20;
 p=read(argv[2],n);if(!p)return 20;const auto loaded=world.load(p,n,0xabcdef01);memset(p,0xff,n);free(p);if(loaded!=WorldLoadError::ok)return 20;
 WorkerUpkeepStage stage{1,false,false};if(argc==4){if(!strcmp(argv[3],"inspection"))stage.upkeep_state=0;else if(!strcmp(argv[3],"editor"))stage.editor_active=true;else if(!strcmp(argv[3],"control"))stage.control_upkeep=true;else return 20;}
 const auto&g=world.game();const Range ranges[]={{&world,sizeof world},{world.original_bytes(),world.original_size()},{world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile)},{world.base(0),(size_t)g.base_count*sizeof(OriginalBase)},{world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&technologies,sizeof technologies},{&units,sizeof units},{&basic,sizeof basic},{&resources,sizeof resources},{&stage,sizeof stage}};
 size_t total=0;for(const auto&r:ranges){if(r.n>SIZE_MAX-total)return 20;total+=r.n;}auto*before=(uint8_t*)malloc(total);if(!before)return 20;size_t at=0;for(const auto&r:ranges){if(r.n)memcpy(before+at,r.p,r.n);at+=r.n;}
 WorkerUpkeepCandidate output;memset(&output,0x5a,sizeof output);WorkerUpkeepCandidate sentinel;memcpy(&sentinel,&output,sizeof output);
 auto call=[&](size_t id,WorkerUpkeepCandidate&out){return world_worker_upkeep_candidate(world,technologies,units,basic,resources,id,stage,out);};
 for(const auto&r:ranges){if(!r.n)continue;const size_t offsets[]={0,4,(r.n-1)&~size_t(3)};for(size_t offset:offsets)if(call(0,*reinterpret_cast<WorkerUpkeepCandidate*>(const_cast<uint8_t*>(static_cast<const uint8_t*>(r.p))+offset))!=StableIntakeError::bad_buffer)return 21;}
 if(call(g.base_count,output)!=StableIntakeError::bad_state||memcmp(&output,&sentinel,sizeof output))return 21;
 for(size_t id=0;id<g.base_count;++id){memset(&output,0x5a,sizeof output);const auto error=call(id,output);if(error!=StableIntakeError::ok&&memcmp(&output,&sentinel,sizeof output))return 21;printf("BASE %lu %d\n",(unsigned long)id,(int)error);if(error!=StableIntakeError::ok)continue;
  const auto&f=output.facts;printf("FACTS %lu %lu %lu %ld %ld %ld %ld %ld %ld %ld %ld %d %d\n",(unsigned long)id,(unsigned long)f.available,(unsigned long)f.worked,(long)f.population,(long)f.specialists,(long)f.nutrient_before,(long)f.mineral_before,(long)f.energy_before,(long)f.efficiency,(long)f.psych_allocation,(long)f.nutrient_threshold,f.can_grow,f.can_riot);
  const auto&a=output.allocation;printf("ALLOC %lu %lu %ld %ld %ld %ld %ld %lu %d %d %d",(unsigned long)id,(unsigned long)a.worked,(long)a.specialists,(long)a.unallocated,(long)a.nutrient_after,(long)a.mineral_after,(long)a.energy_after,(unsigned long)a.added_count,a.reset_overassigned,a.requires_specialist_resolution,a.requires_riot_resolution);for(uint32_t value:a.added)printf(" %lu",(unsigned long)value);puts("");
  printf("YIELDS %lu",(unsigned long)id);for(const auto&row:f.yields)for(int32_t value:row)printf(" %ld",(long)value);puts("");
  const auto&i=output.intake;printf("INTAKE %lu",(unsigned long)id);for(int32_t value:i.satellites)printf(" %ld",(long)value);for(int32_t value:i.tiles)printf(" %ld",(long)value);for(int32_t value:i.intake)printf(" %ld",(long)value);printf(" %lu %lu %ld %ld %ld",(unsigned long)i.worked_tiles,(unsigned long)i.state_flags,(long)i.raw_eco_damage,(long)i.terraform_reduction_after,(long)i.base_energy_after);for(const auto&row:i.tile_yields)for(int32_t value:row)printf(" %ld",(long)value);puts("");
 }
 at=0;for(const auto&r:ranges){if(r.n&&memcmp(before+at,r.p,r.n))return 21;at+=r.n;}free(before);return world.runtime_seed()==0xabcdef01?0:21;
}
