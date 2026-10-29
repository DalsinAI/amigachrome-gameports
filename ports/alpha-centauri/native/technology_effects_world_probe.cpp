/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "technology_effects.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static alpha::Rules rules;
static uint8_t* read(const char* name,size_t& n){FILE* f=fopen(name,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<=0 || size>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)size);if(!p){fclose(f);return nullptr;}const bool ok=fread(p,1,(size_t)size,f)==(size_t)size;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)size;return p;}
int main(int argc,char** argv){
 if(argc!=3)return 20;
 size_t n=0;auto* data=read(argv[1],n);if(!data)return 20;const auto parsed=alpha::read_technologies(data,n,rules);free(data);if(parsed!=alpha::Error::ok)return 20;
 data=read(argv[2],n);if(!data)return 20;alpha::OriginalWorld world;const auto loaded=world.load(data,n,123);if(loaded!=alpha::WorldLoadError::ok){free(data);return 20;}
 alpha::OriginalWorld baseline;if(baseline.load(data,n,123)!=alpha::WorldLoadError::ok)return 21;
 for(int32_t f=0;f<8;++f){alpha::TechnologyEffectsResult r{11,22,33,44,55},before=r;const auto error=alpha::world_technology_effects(world,rules,f,r);if(error!=alpha::LogicError::ok && memcmp(&r,&before,sizeof(r)))return 21;
  printf("EFFECTS %ld %d %ld %ld %ld %ld %ld\n",(long)f,(int)error,(long)r.commerce,(long)r.fungus_nutrient,(long)r.fungus_mineral,(long)r.fungus_energy,(long)r.fungus_unused);
 }
 if(!alpha_test::same_world(world,baseline) || world.runtime_seed()!=123 || world.original_size()!=n || memcmp(world.original_bytes(),data,n))return 21;
 alpha::OriginalWorld absent;alpha::TechnologyEffectsResult r{11,22,33,44,55},before=r;
 const int32_t invalid[]={-1,8};for(int32_t faction:invalid)if(alpha::world_technology_effects(world,rules,faction,r)!=alpha::LogicError::bad_state || memcmp(&r,&before,sizeof(r)))return 21;
 if(alpha::world_technology_effects(absent,rules,1,r)!=alpha::LogicError::bad_state || memcmp(&r,&before,sizeof(r)))return 21;
 free(data);return 0;
}
