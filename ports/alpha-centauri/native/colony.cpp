/* SPDX-License-Identifier: GPL-3.0-or-later. See colony.h for attribution. */
#include "colony.h"
namespace alpha {
static bool fits(int64_t n){return n>=(-2147483647LL-1) && n<=2147483647LL;}
LogicError calculate_human_cost_factors(const BasicRules& rules,int32_t map_size,
 int32_t growth,int32_t industry,HumanCostFactors& output) {
 if(map_size<0 || map_size>6 || rules.nutrient_cost_multi<1 || rules.nutrient_cost_multi>100 ||
    rules.mineral_cost_multi<1 || rules.mineral_cost_multi>100)return LogicError::bad_state;
 int32_t n=rules.nutrient_cost_multi,m=rules.mineral_cost_multi;
 if(map_size==0){n=n*8/10;m=m*8/10;}else if(map_size==1){n=n*9/10;m=m*9/10;}
 growth=growth< -2?-2:growth>5?5:growth;n=(n*(10-growth)+9)/10;
 if(industry<=-3)m=(13*m+9)/10;
 else if(industry==-2)m=(6*m+4)/5;
 else if(industry==-1)m=(11*m+9)/10;
 else if(industry==1)m=(9*m+9)/10;
 else if(industry==2)m=(4*m+4)/5;
 else if(industry==3)m=(7*m+9)/10;
 else if(industry==4)m=(3*m+4)/5;
 else if(industry>=5)m=(m+1)/2;
 // Thinker explicitly guarantees at least one; OpenSMACX can return zero
 // for tiny modded multipliers on small maps. Record this patched behavior.
 output={n<1?1:n,m<1?1:m};return LogicError::ok;
}
LogicError initial_land_yield(const ResourceInfo& resources,const InitialLandTile& t,
 const InitialYieldContext& c,ResourceValues& output) {
 if(t.rockiness>2 || t.moisture>2 || c.base_count<1 || c.base_count>512 ||
    c.economy< -10 || c.economy>10)return LogicError::bad_state;
 for(const auto& r:resources.rows)if(r.nutrients<0 || r.nutrients>100 || r.minerals<0 ||
   r.minerals>100 || r.energy<0 || r.energy>100)return LogicError::bad_state;
 int32_t n,m,e;
 if(t.base){
  const auto& r=resources.rows[1];n=r.nutrients;m=r.minerals;e=r.energy+c.headquarters;
  if(c.economy>4)e+=4;else if(c.economy>3)e+=2;
  else if(c.economy>=0)e+=(c.economy==1);
  else {e+=c.economy;if(!c.headquarters || c.economy< -1 || c.base_count==1)++e;}
 }else if(t.forest){const auto& r=resources.rows[3];n=r.nutrients;m=r.minerals;e=r.energy;}
 else {n=t.rockiness==2?0:t.moisture;m=t.rockiness>0?1:0;e=0;}
 if(t.river)++e;
 if(c.economy>=2)++e;
 if(t.base){e=e<0?0:e>99?99:e;}
 else {if(!c.unlocked[0] && n>2)n=2;if(!c.unlocked[1] && m>2)m=2;if(!c.unlocked[2] && e>2)e=2;}
 output={n,m,e,0};return LogicError::ok;
}
LogicError begin_population_growth(const PopulationInput& in,PopulationTransition& output) {
 if(in.population<1 || in.population>127 || in.nutrient_factor<1 || in.nutrient_factor>1000 ||
   in.population_limit< -127 || in.population_limit>255 || in.nutrient_per_citizen<0 ||
   in.nutrient_per_citizen>100)return LogicError::bad_state;
 PopulationTransition r{in.population,in.accumulated,false,false,false,-1};
 const int32_t cost=(in.population+1)*in.nutrient_factor;
 const bool allowed=in.population<in.population_limit || in.habitation_dome;
 if(in.accumulated>=0) {
  if((in.growth>=6 || in.cloning_vats) && in.nutrient_per_citizen &&
    in.surplus>=in.nutrient_per_citizen && allowed){
    if(r.population<127)++r.population;
    r.recomputed_accumulation_ceiling=cost;
    r.recompute=true;output=r;return LogicError::ok;
  }
  if(in.accumulated>=cost){
    if(in.growth<=-3){r.accumulated=cost;output=r;return LogicError::ok;}
    if(allowed){if(r.population<127)++r.population;r.accumulated=0;r.recompute=true;r.append_recomputed_surplus=true;output=r;return LogicError::ok;}
    r.accumulated=cost/2;
  }
  int64_t total=(int64_t)r.accumulated+in.surplus;if(!fits(total))return LogicError::overflow;
  r.accumulated=(int32_t)total;
 }else {
  if(in.full_turn && (r.population>1 || !in.objective)){
    --r.population;if(r.population<=0){r.removed=true;output=r;return LogicError::ok;}
  }
  r.accumulated=in.surplus;
 }
 output=r;return LogicError::ok;
}
LogicError finish_population_growth(const PopulationTransition& pending,int32_t accumulated,int32_t surplus,
 PopulationTransition& output){
 if(!pending.recompute || pending.removed || pending.population<1 || pending.population>127 ||
   pending.recomputed_accumulation_ceiling< -1 || pending.recomputed_accumulation_ceiling>128000 ||
   (pending.append_recomputed_surplus && pending.recomputed_accumulation_ceiling>=0))
   return LogicError::bad_state;
 auto r=pending;r.accumulated=accumulated;
 if(r.recomputed_accumulation_ceiling>=0 && r.accumulated>r.recomputed_accumulation_ceiling)
   r.accumulated=r.recomputed_accumulation_ceiling;
 if(r.append_recomputed_surplus){int64_t total=(int64_t)r.accumulated+surplus;if(!fits(total))return LogicError::overflow;r.accumulated=(int32_t)total;}
 r.recompute=false;r.append_recomputed_surplus=false;r.recomputed_accumulation_ceiling=-1;output=r;return LogicError::ok;
}
}
