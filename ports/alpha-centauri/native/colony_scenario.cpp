/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "colony_scenario.h"
namespace alpha {
static unsigned workers(uint32_t mask){unsigned n=0;for(;mask;mask>>=1)n+=mask&1u;return n;}
LogicError colony_work_index(const ColonyMap& map,const EarlyColony& colony,unsigned slot,size_t& index){
 const int32_t dx[]={1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1};
 const int32_t dy[]={-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3};
 if(slot>=20)return LogicError::bad_state;
 size_t base;if(movement_tile_index(map.movement.geometry,colony.x,colony.y,base)!=LogicError::ok)return LogicError::bad_state;
 int32_t x;if(movement_wrap_x(map.movement.geometry,colony.x+dx[slot],x)!=LogicError::ok)return LogicError::bad_state;
 size_t result;if(movement_tile_index(map.movement.geometry,x,colony.y+dy[slot],result)!=LogicError::ok ||
   result>=map.movement.count || result>=map.count)return LogicError::bad_state;
 index=result;return LogicError::ok;
}
LogicError early_colony_yields(const ColonyMap& map,const EarlyColony& colony,
 const ResourceInfo& resources,const BasicRules& rules,ColonyYield& output){
 if(colony.removed || colony.population<1 || colony.population>127 || colony.workers&~0xfffffu ||
   workers(colony.workers)>(unsigned)colony.population || !map.moisture || !map.movement.tiles ||
   map.count!=map.movement.count)return LogicError::bad_state;
 size_t base;if(movement_tile_index(map.movement.geometry,colony.x,colony.y,base)!=LogicError::ok ||
   base>=map.count)return LogicError::bad_state;
 ColonyYield result{};InitialYieldContext ctx{0,1,true,{false,false,false}};
 if(map.movement.geometry.width<8 || map.movement.geometry.height<5)return LogicError::bad_state;
 for(int slot=-1;slot<20;++slot){
  if(slot>=0 && !(colony.workers&(1u<<slot)))continue;
  size_t index=base;
  if(slot>=0 && colony_work_index(map,colony,(unsigned)slot,index)!=LogicError::ok)return LogicError::bad_state;
  auto& tile=map.movement.tiles[index];
  if((slot>=0 && (tile.items&1u)) || tile.altitude<0x60 || tile.items&~(1u|4u|8u|0x80u|0x200000u) || map.moisture[index]>2 ||
    (tile.rockiness!=0 && tile.rockiness!=0x40 && tile.rockiness!=0x80))return LogicError::bad_state;
  InitialLandTile land{slot<0,(tile.items&0x200000u)!=0,(tile.items&0x80u)!=0,(uint8_t)(tile.rockiness/64),map.moisture[index]};
  ResourceValues yield;auto error=initial_land_yield(resources,land,ctx,yield);if(error!=LogicError::ok)return error;
  result.totals.nutrients+=yield.nutrients;result.totals.minerals+=yield.minerals;result.totals.energy+=yield.energy;
 }
 NutrientInput input{colony.population,rules.nutrient_req_citizen,result.totals.nutrients,colony.food,0,0,0,1,0,false,false};
 auto error=calculate_nutrients(input,result.nutrients);if(error!=LogicError::ok)return error;
 output=result;return LogicError::ok;
}
LogicError early_colony_toggle_worker(const ColonyMap& map,const ResourceInfo& resources,
 const BasicRules& rules,unsigned slot,EarlyColony& colony){
 if(slot>=20)return LogicError::bad_state;
 auto updated=colony;updated.workers^=1u<<slot;ColonyYield yield;
 auto error=early_colony_yields(map,updated,resources,rules,yield);if(error!=LogicError::ok)return error;
 updated.food=yield.nutrients.accumulated;colony=updated;return LogicError::ok;
}
LogicError early_colony_food_turn(const ColonyMap& map,const ResourceInfo& resources,
 const BasicRules& rules,EarlyColony& colony,ColonyYield& output){
 ColonyYield before;auto error=early_colony_yields(map,colony,resources,rules,before);if(error!=LogicError::ok)return error;
 HumanCostFactors costs;error=calculate_human_cost_factors(rules,2,0,0,costs);if(error!=LogicError::ok)return error;
 PopulationInput in{colony.population,before.nutrients.accumulated,before.nutrients.surplus,costs.nutrient,0,
   rules.pop_limit_sans_hab_complex,rules.nutrient_req_citizen,false,false,true,false};
 PopulationTransition transition;error=begin_population_growth(in,transition);if(error!=LogicError::ok)return error;
 auto updated=colony;updated.population=transition.population;updated.food=transition.accumulated;
 if(transition.removed){updated.removed=true;updated.workers=0;colony=updated;output={};return LogicError::ok;}
 for(int slot=19;slot>=0 && workers(updated.workers)>(unsigned)updated.population;--slot)updated.workers&=~(1u<<slot);
 if(transition.recompute){ColonyYield recomputed;error=early_colony_yields(map,updated,resources,rules,recomputed);if(error!=LogicError::ok)return error;
  PopulationTransition finished;error=finish_population_growth(transition,recomputed.nutrients.accumulated,recomputed.nutrients.surplus,finished);if(error!=LogicError::ok)return error;
  updated.food=finished.accumulated;
 }
 ColonyYield after;error=early_colony_yields(map,updated,resources,rules,after);if(error!=LogicError::ok)return error;
 updated.food=after.nutrients.accumulated;colony=updated;output=after;return LogicError::ok;
}
}
