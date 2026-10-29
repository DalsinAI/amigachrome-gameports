/* SPDX-License-Identifier: GPL-3.0-or-later. See production_cost.h. */
#include "production_cost.h"
namespace alpha {
LogicError unit_production_cost(const UnitProductionCostInput& in,UnitProductionCostResult& output){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) ||
 in.design_id<0 || in.design_id>=512 || in.stored_cost<0 || in.stored_cost>255 ||
 in.component_base_cost<0 || in.triad<0 || in.triad>2 || in.offense<-128 || in.offense>127 ||
 in.plan<0 || in.plan>255 || in.design_owner_difficulty<0 || in.design_owner_difficulty>5 ||
 in.prototype_land_percent<0 || in.prototype_land_percent>1000 ||
 in.prototype_sea_percent<0 || in.prototype_sea_percent>1000 ||
 in.prototype_air_percent<0 || in.prototype_air_percent>1000)return LogicError::bad_state;
 int32_t cost=in.stored_cost,surcharge=0;
 if(in.variant==Variant::alien_crossfire && in.design_id<64 && (in.offense<0 || in.design_id==15)){
  if(!in.base_present)return LogicError::bad_state;
  if(in.physical_brood_pit)cost=cost*3/4;
 }
 if(in.plan==8 && in.base_present && cost==0)cost=1;
 if(in.design_id>=64 && !in.prototype_complete){
  int32_t percent=0;
  if(!in.design_owner_free_prototypes && in.design_owner_difficulty>=2)
   percent=in.triad==1?in.prototype_sea_percent:in.triad==2?in.prototype_air_percent:in.prototype_land_percent;
  const int64_t numerator=(int64_t)in.component_base_cost*percent+50;
  if(numerator>2147483647)return LogicError::overflow;
  surcharge=(int32_t)numerator/100;
  // Original computes base_cost and the intermediate BEFORE Skunkworks clears it.
  if(in.base_present && in.physical_skunkworks)surcharge=0;
  if((int64_t)cost+surcharge>2147483647)return LogicError::overflow;
  cost+=surcharge;
 }
 output=UnitProductionCostResult{cost,surcharge,surcharge!=0};return LogicError::ok;
}
LogicError production_mineral_requirement(int32_t cost,int32_t factor,int32_t& output){
 if(cost<0 || factor<0)return LogicError::bad_state;
 const int64_t n=(int64_t)cost*factor;if(n>2147483647)return LogicError::overflow;
 output=(int32_t)n;return LogicError::ok;
}
}
