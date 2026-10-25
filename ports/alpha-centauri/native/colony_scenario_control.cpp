/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "colony_scenario.h"
#include <stdio.h>
#include <string.h>
int main(){
 static alpha::MovementTile tiles[96];static uint8_t moisture[96];
 for(unsigned i=0;i<96;++i){tiles[i]={0,0x60,0};moisture[i]=2;}tiles[34].items=1;
 alpha::ColonyMap map{{{16,12,false},tiles,96},moisture,96};
 alpha::ResourceInfo resources{};for(auto& r:resources.rows)r={1,1,1,0};resources.rows[1]={2,1,1,0};resources.rows[3]={1,2,1,0};
 alpha::BasicRules rules{};rules.nutrient_req_citizen=2;rules.nutrient_cost_multi=rules.mineral_cost_multi=10;rules.pop_limit_sans_hab_complex=7;
 alpha::EarlyColony colony{4,4,1,0,1u<<6,false};alpha::ColonyYield yield;
 if(alpha::early_colony_yields(map,colony,resources,rules,yield)!=alpha::LogicError::ok || yield.nutrients.intake!=4 || yield.nutrients.surplus!=2)return 1;
 for(unsigned turn=0;turn<10;++turn)if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=1 || colony.food!=(int32_t)(2*(turn+1)))return 2;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=2 || colony.food!=0 || yield.nutrients.surplus!=0)return 3;
 if(alpha::early_colony_toggle_worker(map,resources,rules,0,colony)!=alpha::LogicError::ok || colony.workers!=65)return 4;
 for(unsigned turn=0;turn<15;++turn)if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=2 || colony.food!=(int32_t)(2*(turn+1)))return 5;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=3 || colony.food!=0 || yield.nutrients.surplus!=0)return 6;
 if(alpha::early_colony_toggle_worker(map,resources,rules,0,colony)!=alpha::LogicError::ok || alpha::early_colony_toggle_worker(map,resources,rules,6,colony)!=alpha::LogicError::ok)return 7;
 if(colony.food!=-1)return 8;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=2 || colony.food!=-4)return 9;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=1 || colony.food!=0)return 10;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || colony.population!=1 || colony.food!=0)return 11;
 auto before=colony;tiles[26].items=0x20; // slot0 at(5,3)
 if(alpha::early_colony_toggle_worker(map,resources,rules,0,colony)!=alpha::LogicError::bad_state || memcmp(&before,&colony,sizeof(colony)))return 12;
 if(alpha::early_colony_toggle_worker(map,resources,rules,20,colony)!=alpha::LogicError::bad_state || memcmp(&before,&colony,sizeof(colony)))return 13;
 rules.nutrient_req_citizen=3;
 if(alpha::early_colony_food_turn(map,resources,rules,colony,yield)!=alpha::LogicError::ok || !colony.removed || colony.population || colony.workers)return 14;
#ifndef ALPHA_CONTROL_RAW
 puts("colony_turns=31 growth=2 starvation=2 removed=1 invalid=2 rc=0");
#endif
 return 0;
}
