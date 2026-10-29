/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "research_progress.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
namespace alpha {int32_t original_research_cost_reference(const OriginalResearchCostFacts&);}
static bool read_word(FILE*f,uint32_t&out){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;out=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static void word(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static __attribute__((noinline)) void*alias_pointer(void*p){return p;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t count;if(!read_word(f,count)||count>100000)return 20;
 for(uint32_t i=0;i<count;++i){uint32_t w[36];for(auto&n:w)if(!read_word(f,n))return 20;OriginalResearchCostFacts c{};unsigned p=0;c.faction=(int32_t)w[p++];c.difficulty=(int32_t)w[p++];c.current_turn=(int32_t)w[p++];c.discovery_rate_pct=(int32_t)w[p++];c.rule_techcost=(int32_t)w[p++];c.research_base=(int32_t)w[p++];c.map_tiles=(int32_t)w[p++];for(auto&n:c.ranking)n=(int32_t)w[p++];for(auto&n:c.deferred_techs)n=(int32_t)w[p++];c.ranking_debt=(int32_t)w[p++];c.stagnation=w[p++]!=0;
  ResearchContributionFacts a{(int32_t)w[p],(int32_t)w[p+1],(int32_t)w[p+2],(int32_t)w[p+3],(int32_t)w[p+4],{(int32_t)w[p+5],(int32_t)w[p+6],(int32_t)w[p+7]},w[p+8]!=0,w[p+9]!=0,w[p+10]!=0};const auto oldc=c;const auto olda=a;
  if(original_research_cost(c,*static_cast<OriginalResearchCostResult*>(alias_pointer(&c)))!=ResearchError::bad_buffer||memcmp(&c,&oldc,sizeof c))return 21;
  if(research_contribution(a,*static_cast<ResearchContributionResult*>(alias_pointer(&a)))!=ResearchError::bad_buffer||research_labs(a,*static_cast<ResearchContributionResult*>(alias_pointer(&a)))!=ResearchError::bad_buffer||memcmp(&a,&olda,sizeof a))return 21;
  OriginalResearchCostResult r,before;memset(&r,0x5a,sizeof r);memcpy(&before,&r,sizeof r);const auto e=original_research_cost(c,r);if(e!=ResearchError::ok&&memcmp(&r,&before,sizeof r))return 21;if(e==ResearchError::ok&&r.cost!=original_research_cost_reference(c))return 22;
  word((uint32_t)e);word((uint32_t)r.cost);word((uint32_t)r.map_root);word((uint32_t)r.player_factor);word((uint32_t)r.top_factor);word((uint32_t)r.difficulty_factor);word((uint32_t)r.final_factor);word((uint32_t)r.discovery_rate);
  ResearchContributionResult result,unchanged;memset(&result,0x5a,sizeof result);memcpy(&unchanged,&result,sizeof result);const auto error=research_contribution(a,result);if(error!=ResearchError::ok&&memcmp(&result,&unchanged,sizeof result))return 21;
  word((uint32_t)error);word((uint32_t)result.after.labs_total);word((uint32_t)result.after.rounding_remainder);word((uint32_t)result.after.accumulated);word((uint32_t)result.labs_accepted);word((uint32_t)result.research_points);word(error==ResearchError::ok?(result.completion?1:0):0x5a5a5a5au);word((uint32_t)result.suppression);
  if(memcmp(&c,&oldc,sizeof c)||memcmp(&a,&olda,sizeof a))return 21;
 }
 const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;
}
