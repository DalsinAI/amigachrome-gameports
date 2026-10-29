/* SPDX-License-Identifier: GPL-3.0-or-later. Test-only explicit proposal encoding. */
#include "world_production_prefix.h"
#include "local_actions_test_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t* read(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);if(z<=0||z>64L*1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc((size_t)z);if(!p){fclose(f);return nullptr;}bool ok=fread(p,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(p);return nullptr;}n=(size_t)z;return p;}
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sint(uint32_t v){int32_t n;memcpy(&n,&v,4);return n;}
static bool put(uint32_t v){uint8_t b[4];for(auto&x:b){x=(uint8_t)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
static alpha::Rules rules,before_rules;
static alpha::ProductionPrefixResult result,before_result;
static uint8_t wire[0x20cc];
int main(int argc,char**argv){
 using namespace alpha;if(argc!=5 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 const Variant profile=!strcmp(argv[1],"base")?Variant::base:Variant::alien_crossfire;size_t n;auto*text=read(argv[2],n);if(!text)return 20;
 if(read_technologies(text,n,rules)!=Error::ok)return 20;
 free(text);memcpy(&before_rules,&rules,sizeof rules);auto*save=read(argv[3],n);if(!save)return 20;
 OriginalWorld world,baseline;if(world.load(save,n,123)!=WorldLoadError::ok||baseline.load(save,n,123)!=WorldLoadError::ok)return 20;
 const bool old=world.game().data_version<11;const size_t fs=old?0x20bc:0x20cc;FILE*q=fopen(argv[4],"rb");if(!q)return 20;uint8_t bytes[4];unsigned queries=0;
 for(;;){const size_t count=fread(bytes,1,4,q);if(!count){if(ferror(q))return 20;break;}if(count!=4 || ++queries>10000)return 20;
  const int32_t phase=sint(get(bytes));memset(&result,90,sizeof result);const auto e=world_production_prefix(world,rules,profile,phase,result);
  if(!put((uint32_t)e))return 20;
  if(e!=LogicError::ok){const auto*p=(const uint8_t*)&result;for(size_t i=0;i<sizeof result;++i)if(p[i]!=90)return 21;memset(wire,90,fs);if(fwrite(wire,1,fs,stdout)!=fs)return 20;for(unsigned i=0;i<9;++i)if(!put(0x5a5a5a5a))return 20;continue;}
  memcpy(wire,world.game().factions.data+(size_t)phase*fs,fs);const auto&f=result.faction;
  auto word=[&](size_t offset,uint32_t v){if(old && offset>=0x464)offset-=4;for(unsigned i=0;i<4;++i){wire[offset+i]=(uint8_t)v;v>>=8;}};
  word(0,f.player_flags);word(0x220,(uint32_t)f.energy_credits);
  for(unsigned i=0;i<8;++i)word(0xc4+4*i,(uint32_t)f.loan_balance[i]);
  word(0x2fc,(uint32_t)f.energy_surplus_total);word(0x300,(uint32_t)f.facility_maint_total);word(0x304,(uint32_t)f.tech_commerce_bonus);word(0x308,(uint32_t)f.turn_commerce_income);
  word(0x314,(uint32_t)f.tech_fungus_nutrient);word(0x318,(uint32_t)f.tech_fungus_mineral);word(0x31c,(uint32_t)f.tech_fungus_energy);word(0x320,(uint32_t)f.tech_fungus_unused);word(0x3f8,(uint32_t)f.best_mineral_output);
  for(unsigned i=0;i<8;++i)word(0x690+4*i,(uint32_t)f.social_support[i]);
  for(unsigned i=0;i<8;++i)for(unsigned j=0;j<9;++j)word(0x6b0+4*(i*9+j),(uint32_t)f.social_psych[i][j]);
  for(unsigned i=0;i<9;++i)word(0x7d0+4*i,(uint32_t)f.social_effic[i]);
  word(0x7f4,(uint32_t)f.unk_45);word(0x7f8,(uint32_t)f.unk_46);word(0x7fc,(uint32_t)f.unk_47);word(0x800,(uint32_t)f.nutrient_surplus_total);word(0x804,(uint32_t)f.labs_total);
  auto view=world.game();view.factions={wire,fs};OriginalFactionState decoded;
  if(original_faction_state(view,0,decoded)!=GameReadError::ok || !alpha_test::same(decoded,result.faction) || result.credits[(unsigned)phase]!=f.energy_credits || result.energy_before_bases!=f.energy_credits)return 21;
  if(fwrite(wire,1,fs,stdout)!=fs)return 20;
  for(auto v:result.credits)if(!put((uint32_t)v))return 20;
  if(!put((uint32_t)result.energy_before_bases))return 20;
 }
 fclose(q);unsigned guards=0;memset(&result,90,sizeof result);memcpy(&before_result,&result,sizeof result);
 auto bad=[&](const OriginalWorld&w,Variant p,int phase){++guards;return world_production_prefix(w,rules,p,phase,result)==LogicError::bad_state&&!memcmp(&result,&before_result,sizeof result);};OriginalWorld empty;
 if(!bad(empty,profile,1)||!bad(world,profile,-1)||!bad(world,profile,8)||!bad(world,(Variant)99,1)||!bad(world,profile==Variant::base?Variant::alien_crossfire:Variant::base,1))return 21;
 --rules.count;if(!bad(world,profile,1))return 21;++rules.count;
 const void*aliases[]={&world,&rules,world.original_bytes(),world.current_monuments().data,world.tile(0),world.vehicle(0),world.base(0),world.prototype(0),world.faction(0),world.faction_definition(0)};
 for(const void*at:aliases)if(at){++guards;auto&out=*reinterpret_cast<ProductionPrefixResult*>(const_cast<void*>(at));if(world_production_prefix(world,rules,profile,1,out)!=LogicError::bad_state)return 21;}
 if(!put(guards)||!alpha_test::same_world(world,baseline)||memcmp(world.current_monuments().data,baseline.current_monuments().data,world.current_monuments().size)||memcmp(world.original_bytes(),save,n)||memcmp(&rules,&before_rules,sizeof rules)||world.runtime_seed()!=123)return 21;
 free(save);return 0;
}
