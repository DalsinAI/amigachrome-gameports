/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_production_stage.h"
#include "facility_production_snapshot.h"
#include "local_production_session.h"
#include "production_snapshot.h"
#include "world_actions.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
using namespace alpha;
static OriginalWorld world,reload;
static Rules tech;static BasicRules basic;static WorldRules wr;static FacilityDefinitions fac;
static uint8_t* read(const char* path,size_t& n){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}const long size=ftell(f);rewind(f);
 if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}
 auto* bytes=(uint8_t*)malloc((size_t)size);if(!bytes){fclose(f);return nullptr;}
 const bool ok=fread(bytes,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(bytes);return nullptr;}n=(size_t)size;return bytes;
}
static uint32_t u32(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s32(uint32_t n){return n<=0x7fffffffu?(int32_t)n:-1-(int32_t)~n;}
static void put(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
static int leaf(const char* source,const char* dest){
 size_t n=0;auto* bytes=read(source,n);if(!bytes || n%48){free(bytes);return 20;}
 FILE* exists=fopen(dest,"rb");if(exists){fclose(exists);free(bytes);return 20;}
 FILE* f=fopen(dest,"wb");if(!f){free(bytes);return 20;}
 // Every suitably aligned partial input/output overlap must reject atomically.
 alignas(FacilityProductionFacts) uint8_t alias[256];
 for(size_t offset=0;offset<sizeof(FacilityProductionFacts);offset+=alignof(FacilityProductionResult)){
  memset(alias,0x5a,sizeof(alias));auto* input=new(alias)FacilityProductionFacts{};
  auto* out=reinterpret_cast<FacilityProductionResult*>(alias+offset);uint8_t before[256];memcpy(before,alias,sizeof(alias));
  if(facility_production(*input,*out)!=FacilityProductionError::bad_buffer || memcmp(alias,before,sizeof(alias))){fclose(f);free(bytes);return 21;}
 }
 for(size_t at=0;at<n;at+=48){int32_t v[12];for(unsigned i=0;i<12;++i)v[i]=s32(u32(bytes+at+4*i));
  FacilityProductionFacts in{v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],(uint32_t)v[8],v[9]!=0,v[10]!=0,v[11]!=0};
  FacilityProductionResult out;memset(&out,0x5a,sizeof(out));uint8_t prior[sizeof(out)];memcpy(prior,&out,sizeof(out));
  const auto e=facility_production(in,out);uint8_t encoded[32];put(encoded,(uint32_t)e);
  if(e==FacilityProductionError::ok){const uint32_t vals[7]={(uint32_t)out.accumulated,(uint32_t)out.baseline,(uint32_t)out.cost,(uint32_t)out.credited,(uint32_t)out.best_output,out.state_flags,(uint32_t)out.completed};for(unsigned i=0;i<7;++i)put(encoded+4+i*4,vals[i]);}
  else {if(memcmp(&out,prior,sizeof(out))){fclose(f);free(bytes);return 21;}for(unsigned i=0;i<7;++i)put(encoded+4+i*4,0x5a5a5a5au);}
  if(fwrite(encoded,1,sizeof(encoded),f)!=sizeof(encoded)){fclose(f);free(bytes);return 21;}
 }
 const bool ok=fclose(f)==0;free(bytes);return ok?0:21;
}
int main(int argc,char** argv){
 if(argc==4 && !strcmp(argv[1],"--leaf"))return leaf(argv[2],argv[3]);
 if(argc!=9){puts("Usage: probe RULES WORLD BASE FRESH_INTAKE FRESH_SURPLUS COST_FACTOR MODE NEW_OUTPUT");return 20;}
 FILE* exists=fopen(argv[8],"rb");if(exists){fclose(exists);return 20;}
 size_t rn=0,n=0;auto* rules=read(argv[1],rn);auto* bytes=read(argv[2],n);if(!rules || !bytes)return 20;
 if(read_technologies(rules,rn,tech)!=Error::ok || read_basic_rules(rules,rn,tech,Variant::base,basic,wr)!=Error::ok ||
    read_facility_definitions(rules,rn,tech,Variant::base,fac)!=Error::ok || world.load(bytes,n,0xabcdef01u)!=WorldLoadError::ok)return 20;
 const size_t base_id=(size_t)strtoul(argv[3],nullptr,10);const int mode=atoi(argv[7]);
 FreshFacilityMinerals fresh{atoi(argv[4]),atoi(argv[5]),atoi(argv[6])};
 auto* buffer=(uint8_t*)malloc(n);auto* before=(uint8_t*)malloc(n);if(!buffer || !before)return 20;
 if(mode==102 || mode==103){
  LocalProductionSession order;auto e=order.load_local(bytes,n,rules,rn,Variant::base,false,world.game().player,0xabcdef01u);
  FacilityOrderPlan plan;if(e==LocalOrderError::ok)e=order.prepare_facility((int32_t)base_id,mode-100,plan);
  FacilityOrderReceipt applied{};if(e==LocalOrderError::ok)e=order.commit(plan,applied);
  printf("ORDER facility=%d status=%s\n",mode-100,local_order_error_name(e));
  if(e!=LocalOrderError::ok)return 22;
  ProductionSnapshotReceipt encoded{};
  if(encode_production_snapshot(*order.world(),buffer,n,encoded)!=ProductionSnapshotError::ok || world.load(buffer,n,encoded.runtime_seed)!=WorldLoadError::ok)return 21;
 }
 if(mode==1){
  UnitDefinitions defs{};if(read_unit_definitions(rules,rn,tech,Variant::base,defs)!=Error::ok)return 21;
  const auto moved=WorldActions::clear_land_step(world,defs,tech,0,world.game().player,basic.move_rate_roads,3,3);
  printf("MOVE status=%s cost=%lu\n",step_status_name(moved.status),(unsigned long)moved.price);
  if(moved.status!=StepStatus::moved)return 21;
 }
 LocalSnapshotReceipt pre{};
 if(encode_facility_production_snapshot(world,before,n,pre)!=LocalSnapshotError::ok || reload.load(before,n,pre.runtime_seed)!=WorldLoadError::ok)return 21;
 FacilityProductionStage stage(world,tech,basic,fac);FacilityStageReceipt result{};result.base=-123;
 if(world.base(base_id)){
  auto* alias=reinterpret_cast<FacilityStageReceipt*>(const_cast<OriginalBase*>(world.base(base_id)));
  if(stage.apply(base_id,fresh,*alias)!=FacilityProductionError::bad_buffer)return 21;
  auto* input_alias=reinterpret_cast<const FreshFacilityMinerals*>(world.base(base_id));
  if(stage.apply(base_id,*input_alias,result)!=FacilityProductionError::bad_buffer || result.base!=-123)return 21;
 }
 FacilityStageReceipt untouched=result;
 const auto e=stage.apply(base_id,fresh,result);
 if(e!=FacilityProductionError::ok && (memcmp(&result,&untouched,sizeof(result)) || !alpha_test::same_world(world,reload,false)))return 21;
 auto repeat=FacilityProductionError::bad_state;
 if(e==FacilityProductionError::ok){
  FacilityStageReceipt again{};repeat=stage.apply(base_id,fresh,again);
  if(repeat!=FacilityProductionError::already_processed)return 21;
 }
 if(mode>=2 && mode<=8){
  if(mode==2)++const_cast<OriginalBase*>(world.base(0))->nutrients_accumulated;
  if(mode==3)++const_cast<OriginalFactionState*>(world.faction(1))->energy_credits;
  if(mode==4)const_cast<OriginalBase*>(world.base(0))->facilities_built[0]^=0x10;
  if(mode==5)++const_cast<OriginalBase*>(world.base(0))->queue_items[1];
  if(mode==6)++const_cast<OriginalGameView&>(world.game()).preferences.CurrentTurn;
 }
 memset(buffer,0xa5,n);LocalSnapshotReceipt receipt{123,0x12345678};
 const auto enc=encode_facility_production_snapshot(world,mode==7?const_cast<uint8_t*>(world.original_bytes()):buffer,n-(mode==8?1u:0u),receipt);
 if(mode>=2 && mode<=8){
  if(enc!=(mode>=7?LocalSnapshotError::bad_buffer:LocalSnapshotError::unsupported_change) || receipt.bytes!=123 || receipt.runtime_seed!=0x12345678)return 21;
  for(size_t i=0;i<n;++i)if(buffer[i]!=0xa5)return 21;
  printf("SNAPSHOT reject=%s unchanged=1\n",local_snapshot_error_name(enc));return 0;
 }
 if(enc!=LocalSnapshotError::ok || reload.load(buffer,n,receipt.runtime_seed)!=WorldLoadError::ok || !alpha_test::same_world(world,reload,false))return 21;
 LocalSnapshotReceipt again{};if(encode_facility_production_snapshot(reload,before,n,again)!=LocalSnapshotError::ok || memcmp(before,buffer,n))return 21;
 FILE* f=fopen(argv[8],"wb");if(!f)return 21;const bool written=fwrite(buffer,1,n,f)==n;const bool closed=fclose(f)==0;if(!written || !closed)return 21;
 printf("STAGE status=%s facility=%ld completed=%d before=%ld after=%ld baseline=%ld cost=%ld credited=%ld best=%ld flags=%08lx repeat=%s recompute=%d choice=%d bytes=%lu\n",
  facility_production_error_name(e),(long)result.facility,result.production.completed,(long)result.before,(long)result.production.accumulated,(long)result.production.baseline,
  (long)result.production.cost,(long)result.production.credited,(long)result.production.best_output,(unsigned long)result.production.state_flags,
  facility_production_error_name(repeat),result.recompute_required,result.production_choice_required,(unsigned long)n);
 free(rules);free(bytes);free(buffer);free(before);return 0;
}
