/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_cost.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules tech;static alpha::BasicRules rules;static alpha::WorldRules map_rules;
static uint8_t* read(const char* p,size_t& size){FILE* f=fopen(p,"rb");if(!f)return nullptr;if(fseek(f,0,2)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);if(n<=0||n>64*1024*1024){fclose(f);return nullptr;}auto* b=(uint8_t*)malloc((size_t)n);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(b);return nullptr;}size=(size_t)n;return b;}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;size_t n=0;auto* b=read(argv[2],n);if(!b)return 20;
 bool good=alpha::read_technologies(b,n,tech)==alpha::Error::ok && alpha::read_basic_rules(b,n,tech,variant,rules,map_rules)==alpha::Error::ok;free(b);if(!good)return 20;
 b=read(argv[3],n);if(!b)return 20;alpha::OriginalWorld world;auto loaded=world.load(b,n,0x1234u);free(b);if(loaded!=alpha::WorldLoadError::ok)return 20;
 int32_t root;if(alpha::original_research_map_root((int32_t)world.game().map.info.tile_count,root)!=alpha::LogicError::ok)return 21;
 for(int i=0;i<8;++i){alpha::ResearchCostResult result{17,false,false};auto e=alpha::world_research_cost(world,rules,i,result);printf("COST %d %d %ld %d %d root=%ld\n",i,(int)e,(long)result.cost,result.cached,result.research_disabled,(long)root);}
 alpha::OriginalWorld absent;alpha::ResearchCostResult result{17,false,false};if(alpha::world_research_cost(absent,rules,1,result)!=alpha::LogicError::bad_state || result.cost!=17)return 21;
 if(alpha::world_research_cost(world,rules,-1,result)!=alpha::LogicError::bad_state || result.cost!=17 || alpha::world_research_cost(world,rules,8,result)!=alpha::LogicError::bad_state || result.cost!=17)return 21;
 if(world.runtime_seed()!=0x1234u)return 21;
 return 0;
}
