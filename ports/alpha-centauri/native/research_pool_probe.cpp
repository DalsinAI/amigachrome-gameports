/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_pool.h"
#include <stdio.h>
#include <string.h>
static alpha::Rules rules;static alpha::ResearchOptions options;
static int32_t get(const uint8_t* p){uint32_t n=0;for(unsigned s=0;s<32;s+=8)n|=(uint32_t)p[s/8]<<s;return (int32_t)n;}
static void put(uint32_t n){uint8_t b[4];for(unsigned s=0;s<32;s+=8)b[s/8]=(uint8_t)(n>>s);fwrite(b,1,4,stdout);}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;uint8_t bytes[1800];size_t size,count=0;
 while((size=fread(bytes,1,sizeof(bytes),f))!=0){if(size!=sizeof(bytes) || ++count>100000){fclose(f);return 20;}
  rules.count=89;alpha::HumanResearchContext c{get(bytes),get(bytes+4),get(bytes+8),get(bytes+12)!=0,get(bytes+16)!=0};
  for(unsigned i=0;i<89;++i){const uint8_t* p=bytes+20+i*20;for(unsigned k=0;k<4;++k)rules.technologies[i].priorities[k]=get(p+k*4);options.technologies[i].availability=get(p+16)?alpha::ResearchAvailability::available:alpha::ResearchAvailability::missing_prerequisite;}
  alpha::HumanResearchPool result{};result.count=7;alpha::HumanResearchPool before;memcpy(&before,&result,sizeof(result));auto error=alpha::human_research_pool(rules,options,c,result);
  if(error!=alpha::LogicError::ok && memcmp(&result,&before,sizeof(result))){fclose(f);return 21;}
  put((uint32_t)error);put(error==alpha::LogicError::ok?(uint32_t)result.path:0);put(error==alpha::LogicError::ok?(uint32_t)result.count:0);
  for(unsigned i=0;i<89;++i)put(error==alpha::LogicError::ok && i<result.count?result.ids[i]:0xffffffffu);
  for(unsigned i=0;i<89;++i)put(error==alpha::LogicError::ok && i<result.count?result.categories[i]:0xffffffffu);
  if(error==alpha::LogicError::ok)for(unsigned id=0;id<89;++id){bool present=false;for(size_t j=0;j<result.count;++j)if(result.ids[j]==id)present=true;if(result.offered[id]!=present){fclose(f);return 21;}}
 }
 bool good=!ferror(f);fclose(f);if(!good || !count)return 20;
 /* Invalid source snapshots must not replace the previous result. */
 alpha::HumanResearchContext c{1,0,1,true,false};alpha::HumanResearchPool result{};result.count=7;
 rules.count=88;if(alpha::human_research_pool(rules,options,c,result)!=alpha::LogicError::bad_state || result.count!=7)return 21;rules.count=89;
 options.technologies[0].availability=(alpha::ResearchAvailability)99;if(alpha::human_research_pool(rules,options,c,result)!=alpha::LogicError::bad_state || result.count!=7)return 21;
 return 0;
}
