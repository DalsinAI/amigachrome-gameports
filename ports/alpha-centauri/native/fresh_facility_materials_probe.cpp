/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_production_stage.h"
#include "facility_production_snapshot.h"
#include "world_base_support.h"
#include "world_base_minerals.h"
#include "world_cost_factor.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
static OriginalWorld world,prior,reload;
static Rules tech;static UnitDefinitions units;static BasicRules basic;static WorldRules wr;
static ResourceInfo resources;static FacilityDefinitions facilities;
static uint8_t* read(const char* path,size_t& n){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}
 const long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}
 auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}
 const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;
}
static uint32_t hash(const void* p,size_t n){auto* bytes=(const uint8_t*)p;uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^bytes[i])*16777619u;return h;}
int main(int argc,char** argv){
 if(argc!=6){puts("Usage: fresh-probe RULES WORLD BASE MODE NEW_OUTPUT");return 20;}
 FILE* exists=fopen(argv[5],"rb");if(exists){fclose(exists);return 20;}
 size_t rn=0,n=0;auto* text=read(argv[1],rn);auto* input=read(argv[2],n);if(!text || !input)return 20;
 if(read_technologies(text,rn,tech)!=Error::ok || read_unit_definitions(text,rn,tech,Variant::base,units)!=Error::ok ||
    read_basic_rules(text,rn,tech,Variant::base,basic,wr)!=Error::ok || read_resource_info(text,rn,resources)!=Error::ok ||
    read_facility_definitions(text,rn,tech,Variant::base,facilities)!=Error::ok || world.load(input,n,0xabcdef01u)!=WorldLoadError::ok)return 20;
 const size_t id=(size_t)strtoul(argv[3],nullptr,10);const int mode=atoi(argv[4]);
 if(mode==1){
  if(world.game().vehicle_count<4 || world.game().base_count<2)return 20;
  // Controlled prior engine state, not a new public re-home command. The old
  // raw bytes stay untouched; fresh upkeep must follow these live records.
  for(size_t i=0;i<4;++i)const_cast<OriginalVehicle*>(world.vehicle(i))->home_base_id=1;
 }
 StableIntakeStage intake{Variant::base,false,false,{0,false},0,0};
 if(mode==2)intake.worker.upkeep_state=1;
 if(mode==3)intake.worker.editor_active=true;
 auto* before=(uint8_t*)malloc(n);auto* output=(uint8_t*)malloc(n);
 auto* states=world.game().vehicle_count?(uint32_t*)malloc(world.game().vehicle_count*sizeof(uint32_t)):nullptr;
 if(!before || !output || (world.game().vehicle_count && !states))return 20;
 LocalSnapshotReceipt before_receipt{};
 if(encode_facility_production_snapshot(world,before,n,before_receipt)!=LocalSnapshotError::ok || prior.load(before,n,before_receipt.runtime_seed)!=WorldLoadError::ok)return 21;
 FacilityProductionStage stage(world,tech,basic,facilities);FacilityStageReceipt result{};result.base=-123;
 const void* sources[]={&units,&wr,&resources,&intake,&tech,&basic,&facilities,&stage};
 const size_t sizes[]={sizeof(units),sizeof(wr),sizeof(resources),sizeof(intake),sizeof(tech),sizeof(basic),sizeof(facilities),sizeof(stage)};
 uint32_t original_hashes[8];for(unsigned i=0;i<8;++i)original_hashes[i]=hash(sources[i],sizes[i]);
 for(unsigned i=0;i<8;++i){
  auto* alias=(FacilityStageReceipt*)const_cast<void*>(sources[i]);
  if(stage.apply_fresh_materials(id,units,wr,resources,intake,*alias)!=FacilityProductionError::bad_buffer)return 21;
  auto* partial=(FacilityStageReceipt*)((uint8_t*)const_cast<void*>(sources[i])+4);
  if(stage.apply_fresh_materials(id,units,wr,resources,intake,*partial)!=FacilityProductionError::bad_buffer)return 21;
 }
 for(unsigned i=0;i<8;++i)if(hash(sources[i],sizes[i])!=original_hashes[i])return 21;
 const auto* raw=world.original_bytes();
 if(stage.apply_fresh_materials(id,*reinterpret_cast<const UnitDefinitions*>(raw),wr,resources,intake,result)!=FacilityProductionError::bad_buffer ||
    stage.apply_fresh_materials(id,units,*reinterpret_cast<const WorldRules*>(raw),resources,intake,result)!=FacilityProductionError::bad_buffer ||
    stage.apply_fresh_materials(id,units,wr,*reinterpret_cast<const ResourceInfo*>(raw),intake,result)!=FacilityProductionError::bad_buffer ||
    stage.apply_fresh_materials(id,units,wr,resources,*reinterpret_cast<const StableIntakeStage*>(raw),result)!=FacilityProductionError::bad_buffer || result.base!=-123)return 21;
 StableIntakeStage rejected=intake;rejected.worker.upkeep_state=1;
 if(stage.apply_fresh_materials(id,units,wr,resources,rejected,result)!=FacilityProductionError::unsupported || result.base!=-123 || !alpha_test::same_world(world,prior,false))return 21;
 SupportResult supported{};UpkeepWorldMineralResult mineral{};int32_t factor=-1;
 const auto support_error=world_base_support_policy(world,units,id,supported,states,world.game().vehicle_count);
 auto mineral_error=StableIntakeError::bad_state;
 if(support_error==SupportError::ok){const UpkeepWorldMineralStage ms{intake,supported.mineral_cost};mineral_error=world_base_minerals(world,tech,basic,wr,resources,id,ms,mineral);}
 const auto cost_error=world_base_cost_factor(world,basic,id,1,factor);
 uint8_t result_before[sizeof(result)];memcpy(result_before,&result,sizeof(result));
 const auto error=stage.apply_fresh_materials(id,units,wr,resources,intake,result);
 if(error!=FacilityProductionError::ok && (memcmp(result_before,&result,sizeof(result)) || !alpha_test::same_world(world,prior,false)))return 21;
 FacilityStageReceipt repeated{};const auto again=stage.apply_fresh_materials(id,units,wr,resources,intake,repeated);
 if(again!=(error==FacilityProductionError::ok?FacilityProductionError::already_processed:error))return 21;
 LocalSnapshotReceipt receipt{};
 if(encode_facility_production_snapshot(world,output,n,receipt)!=LocalSnapshotError::ok || reload.load(output,n,receipt.runtime_seed)!=WorldLoadError::ok || !alpha_test::same_world(world,reload,false))return 21;
 LocalSnapshotReceipt encoded{};if(encode_facility_production_snapshot(reload,before,n,encoded)!=LocalSnapshotError::ok || memcmp(before,output,n))return 21;
 FILE* f=fopen(argv[5],"wb");if(!f)return 21;const bool written=fwrite(output,1,n,f)==n;const bool closed=fclose(f)==0;if(!written || !closed)return 21;
 printf("MATERIALS support_error=%u support=%ld mineral_error=%u intake=%ld surplus=%ld cost_error=%u factor=%ld\n",(unsigned)support_error,(long)supported.mineral_cost,(unsigned)mineral_error,(long)mineral.minerals.intake,(long)mineral.minerals.surplus,(unsigned)cost_error,(long)factor);
 printf("FRESH status=%s complete=%d credited=%ld cost=%ld after=%ld best=%ld repeat=%s bytes=%lu\n",facility_production_error_name(error),result.production.completed,(long)result.production.credited,(long)result.production.cost,(long)result.production.accumulated,(long)result.production.best_output,facility_production_error_name(again),(unsigned long)n);
 free(text);free(input);free(before);free(output);free(states);return 0;
}
