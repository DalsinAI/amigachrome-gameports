/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "technology_effects.h"
#include "technology_effects_ranges.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
struct RuleBound {TechnologyEffectsResult prefix;Rules value;TechnologyEffectsResult suffix;};
struct FactBound {TechnologyEffectsResult prefix;TechnologyEffectsFacts value;TechnologyEffectsResult suffix;};
static RuleBound rules;
static FactBound facts;
static void* aligned_tail(const void* p,size_t bytes){return (void*)(((uintptr_t)p+bytes-4)&~(uintptr_t)3);}
static bool pure_controls(unsigned& rejected,unsigned& adjacent,unsigned& ranges){
 memset(&rules,0,sizeof(rules));memset(&facts,0,sizeof(facts));rules.value.count=89;
 for(auto& tech:rules.value.technologies){tech.prerequisites[0]=-1;tech.prerequisites[1]=-1;}
 facts.value.faction=1;facts.value.rule_commerce=3;facts.value.pending_economy=4;
 static uint8_t old_rules[sizeof(rules)],old_facts[sizeof(facts)];memcpy(old_rules,&rules,sizeof(rules));memcpy(old_facts,&facts,sizeof(facts));
 void* targets[]={&rules.value,(uint8_t*)&rules.value-8,&rules.value.technologies[45].flags,aligned_tail(&rules.value,sizeof(Rules)),&facts.value,(uint8_t*)&facts.value-8,facts.value.owners+4,aligned_tail(&facts.value,sizeof(TechnologyEffectsFacts))};
 for(void* p:targets){auto& out=*(TechnologyEffectsResult*)p;
  if(recompute_technology_effects(rules.value,facts.value,out)!=LogicError::bad_state || memcmp(&rules,old_rules,sizeof(rules)) || memcmp(&facts,old_facts,sizeof(facts)))return false;
  ++rejected;
 }
 // Valid distinct Result objects immediately before/after inputs must still
 // work; this detects an over-broad or <= boundary overlap rejection.
 TechnologyEffectsResult* outputs[]={&rules.prefix,&rules.suffix,&facts.prefix,&facts.suffix};
 for(auto* out:outputs){
  if(recompute_technology_effects(rules.value,facts.value,*out)!=LogicError::ok || out->commerce!=5 || out->fungus_nutrient || out->fungus_mineral || out->fungus_energy || out->fungus_unused)return false;
  ++adjacent;
 }
 const void* a=(const void*)(uintptr_t)0x1000;const void* b=(const void*)(uintptr_t)0x1014;
 struct Test {const void* a;size_t n;const void* b;size_t m;bool expected;};
 const Test tests[]={{a,20,b,20,false},{a,21,b,20,true},{b,20,a,21,true},{a,0,a,20,false},{a,20,a,0,false},{a,20,a,20,true},{(const void*)(UINTPTR_MAX-7),16,a,20,true},{a,20,(const void*)(UINTPTR_MAX-7),16,true}};
 for(const auto& test:tests){if(detail::effects_range_overlap(test.a,test.n,test.b,test.m)!=test.expected)return false;++ranges;}
 if(!detail::effects_array_overlap(a,20,b,SIZE_MAX,2) || detail::effects_array_overlap(a,20,b,0,SIZE_MAX))return false;
 ranges+=2;
 return true;
}
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 unsigned rejected=0,adjacent=0,ranges=0;if(!pure_controls(rejected,adjacent,ranges))return 21;
 size_t n=0;auto* data=read(argv[1],n);if(!data)return 20;
 OriginalWorld world,baseline;if(world.load(data,n,123)!=WorldLoadError::ok || baseline.load(data,n,123)!=WorldLoadError::ok)return 20;
 static uint8_t world_object[sizeof(world)],rules_before[sizeof(rules.value)];memcpy(world_object,&world,sizeof(world));memcpy(rules_before,&rules.value,sizeof(rules.value));
 struct Range {const void* p;size_t bytes;};
 const Range allocations[]={{&world,sizeof(world)},{world.original_bytes(),world.original_size()},{world.tile(0),world.game().map.info.tile_count*sizeof(OriginalMapTile)},{world.vehicle(0),world.game().vehicle_count*sizeof(OriginalVehicle)},{world.base(0),world.game().base_count*sizeof(OriginalBase)},{world.prototype(0),512*sizeof(OriginalPrototype)},{world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},{&rules.value,sizeof(Rules)}};
 unsigned world_rejected=0;
 for(const auto& allocation:allocations){if(!allocation.bytes)continue;
  const uintptr_t start=(uintptr_t)allocation.p;const uintptr_t middle=(start+allocation.bytes/2)&~(uintptr_t)3;
  void* targets[]={(void*)start,(void*)middle,aligned_tail(allocation.p,allocation.bytes)};
  for(void* p:targets){auto& out=*(TechnologyEffectsResult*)p;
   if(world_technology_effects(world,rules.value,1,out)!=LogicError::bad_state || memcmp(&world,world_object,sizeof(world)) || memcmp(&rules.value,rules_before,sizeof(rules.value)) || !alpha_test::same_world(world,baseline))return 21;
   ++world_rejected;
  }
 }
 TechnologyEffectsResult result{11,22,33,44,55};
 if(world_technology_effects(world,rules.value,1,result)!=LogicError::ok || !alpha_test::same_world(world,baseline))return 21;
 printf("ALIASES pure=%u adjacent=%u range=%u world=%u disjoint=1 unchanged=1 RC=0\n",rejected,adjacent,ranges,world_rejected);
 free(data);return 0;
}
