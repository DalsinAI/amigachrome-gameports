/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "technology_effects.h"
#include <stdio.h>
#include <string.h>
static alpha::Rules rules;
static int32_t get(const uint8_t* p){uint32_t n=0;for(unsigned i=0;i<4;++i)n|=(uint32_t)p[i]<<(8*i);return (int32_t)n;}
static bool put(int32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i)b[i]=(uint8_t)((uint32_t)n>>(8*i));return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t data[1504];size_t n,count=0;
 while((n=fread(data,1,sizeof(data),f))!=0){
  if(n!=sizeof(data)||++count>100000){fclose(f);return 20;}
  alpha::TechnologyEffectsFacts facts{};facts.faction=get(data);facts.rule_commerce=get(data+4);facts.pending_economy=get(data+8);facts.bonus_count=get(data+12);
  for(unsigned i=0;i<8;++i){facts.bonus_kind[i]=get(data+16+i*8);facts.bonus_value[i]=get(data+20+i*8);}
  rules.count=89;for(unsigned i=0;i<89;++i){const uint8_t* p=data+80+i*16;rules.technologies[i].flags=(uint32_t)get(p);rules.technologies[i].prerequisites[0]=get(p+4);rules.technologies[i].prerequisites[1]=get(p+8);facts.owners[i]=(uint8_t)get(p+12);}
  alpha::TechnologyEffectsResult result{11,22,33,44,55},before=result;const auto error=alpha::recompute_technology_effects(rules,facts,result);
  if(error!=alpha::LogicError::ok && memcmp(&result,&before,sizeof(result))){fclose(f);return 21;}
  if(!put((int32_t)error))return 20;
  const int32_t fields[]={result.commerce,result.fungus_nutrient,result.fungus_mineral,result.fungus_energy,result.fungus_unused};for(auto value:fields)if(!put(error==alpha::LogicError::ok?value:0))return 20;
 }
 const bool ok=!ferror(f);fclose(f);if(!ok || !count)return 20;
 alpha::TechnologyEffectsFacts facts{};alpha::TechnologyEffectsResult sentinel{11,22,33,44,55},before=sentinel;rules.count=88;
 if(alpha::recompute_technology_effects(rules,facts,sentinel)!=alpha::LogicError::bad_state || memcmp(&sentinel,&before,sizeof(sentinel)))return 21;
 return 0;
}
