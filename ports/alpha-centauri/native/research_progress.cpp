/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in research_progress.h. */
#include "research_progress.h"
namespace alpha {namespace {
bool overlap(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
int32_t clamp(int32_t n,int32_t low,int32_t high){return n<low?low:n>high?high:n;}
int32_t root(int32_t n){if(n<=1)return n;int32_t next=n>>1,current;do{current=next;next=(next+n/next)>>1;}while(next<current);return current;}
}
ResearchError original_research_cost(const OriginalResearchCostFacts&f,OriginalResearchCostResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return ResearchError::bad_buffer;
 if(f.faction<1||f.faction>7||f.difficulty<0||f.difficulty>5||f.current_turn<0||f.discovery_rate_pct<0||f.discovery_rate_pct>1000||f.rule_techcost<0||f.map_tiles<1||f.map_tiles>8388608||f.ranking_debt<0)return ResearchError::bad_state;
 OriginalResearchCostResult r{};r.map_root=root(f.map_tiles);
 if(!f.discovery_rate_pct){r.cost=999999999;out=r;return ResearchError::ok;}
 int32_t score,top=0;
 for(unsigned i=1;i<8;++i){if(f.ranking[i]<0||f.deferred_techs[i]<0)return ResearchError::bad_state;int32_t doubled;if(!put(2LL*f.deferred_techs[i],doubled)||!put((int64_t)doubled+f.ranking[i],score))return ResearchError::overflow;if(score>top)top=score;}
 if(!put(2LL*f.deferred_techs[f.faction],score)||!put((int64_t)score-f.ranking_debt,score)||!put((int64_t)score+f.ranking[f.faction],score))return ResearchError::overflow;
 r.player_factor=clamp(score,2,9999)/2;r.top_factor=top/2;
 int32_t difficulty=f.difficulty+(f.difficulty<3);difficulty=4*difficulty+8;r.difficulty_factor=clamp(difficulty,12-r.player_factor,r.player_factor+12);
 const int32_t period=f.stagnation?12:8;
 const int32_t turn_penalty=clamp(r.player_factor-f.current_turn/period,0,r.difficulty_factor*(f.stagnation?3:2)/2);r.final_factor=turn_penalty+r.difficulty_factor;
 int32_t reduction;if(!put((int64_t)r.top_factor-3-r.player_factor+7,reduction))return ResearchError::overflow;reduction=clamp(reduction/5,0,3*r.final_factor/10+1);
 const int32_t multiplier=clamp(r.player_factor-clamp(f.research_base,-1,1),1,99999);
 if(!put((int64_t)(r.final_factor-reduction)*multiplier,r.discovery_rate))return ResearchError::overflow;
 int32_t product;
 if(f.discovery_rate_pct!=100){if(!put(100LL*r.discovery_rate,product))return ResearchError::overflow;r.discovery_rate=product/f.discovery_rate_pct;}
 if(f.rule_techcost!=100){if(!put((int64_t)r.discovery_rate*f.rule_techcost,product))return ResearchError::overflow;r.discovery_rate=product/100;}
 if(!put((int64_t)r.discovery_rate*r.map_root,product))return ResearchError::overflow;
 r.cost=product/56;
 if(f.stagnation&&!put((int64_t)r.cost+r.cost/2,r.cost))return ResearchError::overflow;
 r.cost=clamp(r.cost,1,99999999);out=r;return ResearchError::ok;
}
ResearchError research_labs(const ResearchContributionFacts&f,ResearchContributionResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return ResearchError::bad_buffer;
 if(f.fresh_labs<0||f.current_turn<0||f.difficulty<0||f.difficulty>5||f.before.labs_total<0||f.before.rounding_remainder<0||f.before.rounding_remainder>99||f.before.accumulated<0)return ResearchError::bad_state;
 ResearchContributionResult r{};r.after=f.before;r.labs_accepted=f.fresh_labs;
 if(f.no_tech_advances)r.suppression=ResearchSuppression::no_tech_advances;
 else if(!f.current_turn)r.suppression=ResearchSuppression::initial_turn;
 else if(f.tutorial_messages&&f.current_turn<=5)r.suppression=ResearchSuppression::tutorial;
 if(r.suppression!=ResearchSuppression::none){r.labs_accepted=0;out=r;return ResearchError::ok;}
 if(f.punishment_sphere)r.labs_accepted/=2;
 int32_t threshold;if(!put(-5LL*f.pending_research,threshold))return ResearchError::overflow;
 if(f.current_turn<threshold){r.labs_accepted=0;r.suppression=ResearchSuppression::negative_research_start;}
 if(f.current_turn<5&&f.difficulty<2){r.labs_accepted=0;r.suppression=ResearchSuppression::low_difficulty_start;}
 if(!put((int64_t)r.after.labs_total+r.labs_accepted,r.after.labs_total))return ResearchError::overflow;
 int32_t product;if(!put(10LL*r.labs_accepted,product)||!put((int64_t)product*(clamp(f.pending_research,-5,5)+10),product))return ResearchError::overflow;
 const int32_t remainder=product%100+f.before.rounding_remainder;
 if(remainder>=100){if(!put((int64_t)product+100,product))return ResearchError::overflow;r.after.rounding_remainder=remainder-100;}else r.after.rounding_remainder=remainder;
 r.research_points=product/100;
 out=r;return ResearchError::ok;
}
ResearchError research_contribution(const ResearchContributionFacts&f,ResearchContributionResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return ResearchError::bad_buffer;
 ResearchContributionResult r{};const auto error=research_labs(f,r);if(error!=ResearchError::ok)return error;
 if(r.research_points){if(f.fresh_cost<1||f.fresh_cost>999999999)return ResearchError::bad_state;if(!put((int64_t)r.after.accumulated+r.research_points,r.after.accumulated))return ResearchError::overflow;if(r.after.accumulated>=f.fresh_cost){r.completion=true;r.after.accumulated=0;}}
 out=r;return ResearchError::ok;
}
}
