/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_base_budget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static OriginalWorld world;
static Rules tech;static UnitDefinitions units;static BasicRules basic;static WorldRules wr;
static ResourceInfo resources;static CitizenDefinitions citizens;static FacilityDefinitions facilities;
static uint8_t* read(const char* path,size_t& n){
 FILE*f=fopen(path,"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}
 const long size=ftell(f);rewind(f);
 if(size<=0||size>64*1024*1024){fclose(f);return nullptr;}
 auto*p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}
 const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);
 if(!ok){free(p);return nullptr;}n=(size_t)size;return p;
}
static void put(uint32_t x){for(unsigned i=0;i<4;++i){putchar((int)(x&255u));x>>=8;}}
static uint32_t hash(const void*p,size_t n){const auto*b=(const uint8_t*)p;uint32_t h=2166136261u;while(n--)h=(h^*b++)*16777619u;return h;}
int main(int argc,char**argv){
 if(argc!=8)return 20;
 size_t rn=0,n=0;auto*text=read(argv[1],rn);auto*data=read(argv[2],n);if(!text||!data)return 20;
 if(read_technologies(text,rn,tech)!=Error::ok||read_unit_definitions(text,rn,tech,Variant::base,units)!=Error::ok||
 read_basic_rules(text,rn,tech,Variant::base,basic,wr)!=Error::ok||read_resource_info(text,rn,resources)!=Error::ok||
 read_citizens(text,rn,tech,citizens)!=Error::ok||read_facility_definitions(text,rn,tech,Variant::base,facilities)!=Error::ok||
 world.load(data,n,0xabcdef01u)!=WorldLoadError::ok)return 20;
 const size_t id=(size_t)strtoul(argv[3],nullptr,10);const int mode=atoi(argv[7]);
 BaseBudgetStage stage{{{Variant::base,false,false,{0,false},0,0},3,0},
  (int32_t)strtol(argv[4],nullptr,10),(int32_t)strtol(argv[5],nullptr,10),(uint32_t)strtoul(argv[6],nullptr,10)};
 if(mode==1)stage.resources.intake.worker.upkeep_state=1;
 if(mode==2)stage.resources.energy_grid=1;
 if(mode==3)stage.resources.commerce=INT32_MAX;
 const size_t count=world.game().vehicle_count;
 auto*states=(uint32_t*)malloc((count+2)*sizeof(uint32_t));if(!states)return 20;
 for(size_t i=0;i<count+2;++i)states[i]=0x5a5a5a5au;
 BaseBudgetResult result;memset(&result,0x5a,sizeof result);
 const uint32_t output_before=hash(&result,sizeof result),world_before=hash(world.original_bytes(),n);
 const void*sources[]={&tech,&units,&basic,&wr,&resources,&citizens,&facilities,&stage};
 const size_t sizes[]={sizeof tech,sizeof units,sizeof basic,sizeof wr,sizeof resources,sizeof citizens,sizeof facilities,sizeof stage};
 for(unsigned i=0;i<8;++i){
  const uint32_t before=hash(sources[i],sizes[i]);
  auto*alias=(BaseBudgetResult*)const_cast<void*>(sources[i]);
  if(world_base_budget(world,tech,units,basic,wr,resources,citizens,facilities,id,stage,*alias,states,count)!=BudgetError::bad_buffer||hash(sources[i],sizes[i])!=before)return 21;
 }
 if(world_base_budget(world,tech,units,basic,wr,resources,citizens,facilities,id,stage,result,(uint32_t*)&result,count)!=BudgetError::bad_buffer)return 21;
 if(count&&world_base_budget(world,tech,units,basic,wr,resources,citizens,facilities,id,stage,result,states,count-1)!=BudgetError::bad_buffer)return 21;
 const auto error=world_base_budget(world,tech,units,basic,wr,resources,citizens,facilities,id,stage,result,states,count);
 if(error!=BudgetError::ok){
  if(hash(&result,sizeof result)!=output_before)return 21;
  for(size_t i=0;i<count;++i)if(states[i]!=0x5a5a5a5au)return 21;
 }
 if(hash(world.original_bytes(),n)!=world_before||states[count]!=0x5a5a5a5au||states[count+1]!=0x5a5a5a5au)return 21;
 put((uint32_t)error);
 if(error==BudgetError::ok){
  const auto&a=result.allocation.allocation;const auto&m=result.maintenance;
  for(int32_t v:{result.resources.energy.accounting.surplus,a.economy,a.psych,a.labs,result.credits_after_income,result.mineral_cost_factor,(int32_t)result.maintenance_applied,m.credits_after,m.maintenance_after})put((uint32_t)v);
  put(m.player_flags_after);
  for(unsigned i=0;i<12;++i)put(m.built_after[i]);
  for(unsigned i=0;i<12;++i)put(m.scrapped[i]);
  for(unsigned i=0;i<12;++i)put(m.shortage[i]);
  for(unsigned i=0;i<70;++i)put((uint32_t)m.charged[i]);
  for(size_t i=0;i<count;++i)put(states[i]);
 }
 free(text);free(data);free(states);return 0;
}
