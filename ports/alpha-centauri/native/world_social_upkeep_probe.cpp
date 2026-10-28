/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only binding harness. */
#include "world_social_upkeep.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t* read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64L*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)z;return p;}
static bool put(uint32_t n){uint8_t b[4];for(auto&v:b){v=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
static alpha::Rules tech;static alpha::SocialRules rules,before_rules;
int main(int argc,char**argv){
 using namespace alpha;if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 Variant profile=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n;auto*text=read(argv[2],n);if(!text)return 20;
 if(read_technologies(text,n,tech)!=Error::ok||read_social_rules(text,n,tech,profile,rules)!=Error::ok)return 20;
 free(text);memcpy(&before_rules,&rules,sizeof rules);auto*save=read(argv[3],n);if(!save)return 20;
 OriginalWorld world,baseline;if(world.load(save,n,123)!=WorldLoadError::ok||baseline.load(save,n,123)!=WorldLoadError::ok)return 20;
 for(int f=1;f<8;++f){SocialUpkeepResult r;memset(&r,90,sizeof r);const auto e=world_social_upkeep(world,rules,profile,f,r);
  if(e!=LogicError::ok){const auto*p=(const uint8_t*)&r;for(size_t i=0;i<sizeof r;++i)if(p[i]!=90)return 21;}
  if(!put((uint32_t)e))return 20;
  for(auto v:r.current)if(!put(v))return 20;
  for(auto v:r.pending)if(!put((uint32_t)v))return 20;
  for(auto v:r.active)if(!put((uint32_t)v))return 20;
  for(auto v:r.temporary)if(!put((uint32_t)v))return 20;
  if(!put((uint32_t)r.upheaval_cost_paid))return 20;
 }
 unsigned guards=0;SocialUpkeepResult out,before;memset(&out,90,sizeof out);memcpy(&before,&out,sizeof out);
 auto bad=[&](const OriginalWorld&w,const SocialRules&r,Variant p,int f){++guards;return world_social_upkeep(w,r,p,f,out)==LogicError::bad_state&&!memcmp(&out,&before,sizeof out);};
 OriginalWorld empty;
 if(!bad(empty,rules,profile,1)||!bad(world,rules,profile,0)||!bad(world,rules,profile,8)||!bad(world,rules,(Variant)99,1)||!bad(world,rules,profile==Variant::base?Variant::alien_crossfire:Variant::base,1))return 21;
 --rules.bonus_count;if(!bad(world,rules,profile,1))return 21;++rules.bonus_count;
 const void*aliases[]={&world,&rules,world.original_bytes(),world.current_monuments().data,world.tile(0),world.vehicle(0),world.base(0),world.prototype(0),world.faction(0),world.faction_definition(0)};
 for(const void*at:aliases)if(at){++guards;auto&r=*reinterpret_cast<SocialUpkeepResult*>(const_cast<void*>(at));if(world_social_upkeep(world,rules,profile,1,r)!=LogicError::bad_state)return 21;}
 if(!put(guards)||!alpha_test::same_world(world,baseline)||memcmp(world.current_monuments().data,baseline.current_monuments().data,world.current_monuments().size)||memcmp(world.original_bytes(),save,n)||memcmp(&rules,&before_rules,sizeof rules)||world.runtime_seed()!=123)return 21;
 free(save);return 0;
}
