/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_cost.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int32_t get(const uint8_t* p){uint32_t n=0;for(unsigned i=0;i<4;++i)n|=(uint32_t)p[i]<<(8*i);return (int32_t)n;}
static void put(uint32_t n){uint8_t p[4];for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i));fwrite(p,1,4,stdout);}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 uint8_t row[224];size_t count=0,n;
 while((n=fread(row,1,sizeof(row),f))!=0){if(n!=sizeof(row) || ++count>100000){fclose(f);return 20;}
  alpha::ResearchCostContext c{};c.discovery_rate=get(row+4);c.map_area_sqrt=get(row+8);c.map_area_sqrt_known=true;c.turn=get(row+12);c.global_difficulty=get(row+16);c.stagnation=(get(row+20)&0x20)!=0;c.human_mask=(uint8_t)get(row+24);c.faction=get(row+28);
  for(unsigned i=0;i<8;++i)c.factions[i]={get(row),get(row+32+i*4),get(row+64+i*4),get(row+96+i*4),get(row+128+i*4),get(row+160+i*4),get(row+192+i*4)};
  alpha::ResearchCostResult result{17,false,false};const alpha::ResearchCostContext before=c;auto error=alpha::original_research_cost(c,result);if(memcmp(&before,&c,sizeof(c))){fclose(f);return 21;}
  put((uint32_t)error);put((uint32_t)result.cost);put(result.cached+2u*result.research_disabled);
 }
 bool good=!ferror(f);fclose(f);if(!good || !count)return 20;
 /* Context validation and unused-input short circuit controls. */
 alpha::ResearchCostContext c{};alpha::ResearchCostResult output{17,false,false};c.faction=8;
 if(alpha::original_research_cost(c,output)!=alpha::LogicError::bad_state || output.cost!=17)return 21;
 c.faction=1;c.factions[1].cached_cost=0;c.discovery_rate=-1;c.map_area_sqrt_known=false;
 if(alpha::original_research_cost(c,output)!=alpha::LogicError::ok || output.cost || !output.cached)return 21;
 c.factions[1].cached_cost=-1;c.discovery_rate=0;
 if(alpha::original_research_cost(c,output)!=alpha::LogicError::ok || output.cost!=999999999 || !output.research_disabled)return 21;
 c.discovery_rate=100;output={17,false,false};
 if(alpha::original_research_cost(c,output)!=alpha::LogicError::bad_state || output.cost!=17)return 21;
 for(int32_t input=0;input<10000;++input){int32_t root=-1;if(alpha::original_research_map_root(input,root)!=alpha::LogicError::ok || (int64_t)root*root>input || (int64_t)(root+1)*(root+1)<=input)return 21;}
 const int32_t bounds[]={1073741824,2147395600,2147483647};for(int32_t input:bounds){int32_t root=-1;if(alpha::original_research_map_root(input,root)!=alpha::LogicError::ok || (int64_t)root*root>input || (int64_t)(root+1)*(root+1)<=input)return 21;}
 int32_t untouched=17;if(alpha::original_research_map_root(-1,untouched)!=alpha::LogicError::bad_state || untouched!=17)return 21;
 return 0;
}
