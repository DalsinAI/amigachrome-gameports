/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_choice.h"
#include <stdio.h>
#include <string.h>
static int32_t get(const unsigned char* p){uint32_t n=0;for(unsigned i=0;i<4;++i)n|=(uint32_t)p[i]<<(8*i);return (int32_t)n;}
static bool put(int32_t n){unsigned char p[4];for(unsigned i=0;i<4;++i)p[i]=(unsigned char)((uint32_t)n>>(8*i));return fwrite(p,1,4,stdout)==4;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");
 if(!f)return 20;
 unsigned char bytes[20];size_t n,count=0;
 while((n=fread(bytes,1,sizeof(bytes),f))!=0){
  if(n!=sizeof(bytes)||++count>100000){fclose(f);return 20;}
  alpha::ManualResearchState input{get(bytes),get(bytes+4),get(bytes+8)};int32_t stage=get(bytes+12),choice=get(bytes+16);
  alpha::ManualResearchResult result{};memset(&result,0xa5,sizeof(result));alpha::ManualResearchResult before;memcpy(&before,&result,sizeof(result));
  auto error=alpha::manual_research_choice(input,(alpha::ManualResearchStage)stage,choice,result);
  if(error!=alpha::ManualResearchError::ok && memcmp(&result,&before,sizeof(result))){fclose(f);return 21;}
  if(error==alpha::ManualResearchError::ok){
   alpha::ManualResearchResult alias{};alias.state=input;
   if(alpha::manual_research_choice(alias.state,(alpha::ManualResearchStage)stage,choice,alias)!=error || alias.state.target!=result.state.target || alias.state.accumulated!=result.state.accumulated || alias.state.cached_cost!=result.state.cached_cost || alias.disposition!=result.disposition || alias.synchronisation_required!=result.synchronisation_required){fclose(f);return 21;}
  }
  int32_t values[7]={(int32_t)error,0,0,0,0,0,(int32_t)alpha::manual_research_confirmation_required(input)};
  if(error==alpha::ManualResearchError::ok){values[1]=result.state.target;values[2]=result.state.accumulated;values[3]=result.state.cached_cost;values[4]=(int32_t)result.disposition;values[5]=result.synchronisation_required;}
  for(auto value:values)if(!put(value)){fclose(f);return 20;}
 }
 bool ok=!ferror(f);fclose(f);return ok&&count?0:20;
}
