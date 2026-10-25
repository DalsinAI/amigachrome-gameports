/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "colony.h"
#include <stdio.h>
#include <initializer_list>
static void word(uint32_t& h,uint32_t n){for(unsigned i=0;i<32;i+=8)h=(h^((n>>i)&255u))*16777619u;}
int main(){
 uint32_t h=2166136261u,costs=0,yields=0,growth=0;
 alpha::BasicRules rules{};
 for(int32_t n:{1,10,100})for(int32_t m:{1,10,100})for(int32_t size=0;size<7;++size)
 for(int32_t g=-5;g<9;++g)for(int32_t i=-5;i<9;++i){
  rules.nutrient_cost_multi=n;rules.mineral_cost_multi=m;alpha::HumanCostFactors out;
  if(alpha::calculate_human_cost_factors(rules,size,g,i,out)!=alpha::LogicError::ok)return 1;
  word(h,(uint32_t)out.nutrient);word(h,(uint32_t)out.mineral);++costs;
 }
 alpha::ResourceInfo info{};for(auto& r:info.rows)r={1,1,1,0};info.rows[1]={2,1,1,0};info.rows[3]={1,2,1,0};
 for(int32_t economy=-5;economy<7;++economy)for(unsigned rock=0;rock<3;++rock)for(unsigned moisture=0;moisture<3;++moisture)
 for(unsigned flags=0;flags<64;++flags){
  alpha::InitialLandTile tile{(flags&1)!=0,(flags&2)!=0,(flags&4)!=0,(uint8_t)rock,(uint8_t)moisture};
  alpha::InitialYieldContext c{economy,flags&8?1:2,(flags&16)!=0,{(flags&32)!=0,(flags&32)!=0,(flags&32)!=0}};
  alpha::ResourceValues out;if(alpha::initial_land_yield(info,tile,c,out)!=alpha::LogicError::ok)return 2;
  word(h,(uint32_t)out.nutrients);word(h,(uint32_t)out.minerals);word(h,(uint32_t)out.energy);++yields;
 }
 for(int32_t pop:{1,2,7,127})for(int32_t accum:{-5,0,19,20,100000})for(int32_t surplus:{-5,0,2,5})
 for(int32_t factor:{1,10})for(int32_t rate:{-3,0,6})for(int32_t limit:{1,7,127})for(unsigned flags=0;flags<16;++flags){
  alpha::PopulationInput in{pop,accum,surplus,factor,rate,limit,2,(flags&1)!=0,(flags&2)!=0,(flags&4)!=0,(flags&8)!=0};
  alpha::PopulationTransition out;
  if(alpha::begin_population_growth(in,out)!=alpha::LogicError::ok)return 3;
  word(h,(uint32_t)out.population);word(h,(uint32_t)out.accumulated);word(h,out.recompute);word(h,out.append_recomputed_surplus);word(h,out.removed);word(h,(uint32_t)out.recomputed_accumulation_ceiling);
  if(out.recompute){auto pending=out;int32_t next_surplus=surplus-(out.population-pop)*2;int32_t adjusted=out.accumulated;
   if(next_surplus>=0 && adjusted<0)adjusted=0;else if(next_surplus<0 && !adjusted)adjusted=-1;
   if(alpha::finish_population_growth(pending,adjusted,next_surplus,out)!=alpha::LogicError::ok)return 4;
   word(h,(uint32_t)out.population);word(h,(uint32_t)out.accumulated);word(h,out.recompute);word(h,out.append_recomputed_surplus);word(h,out.removed);word(h,(uint32_t)out.recomputed_accumulation_ceiling);}
  ++growth;
 }
 alpha::HumanCostFactors costs_out{123,124};rules.nutrient_cost_multi=0;
 if(alpha::calculate_human_cost_factors(rules,2,0,0,costs_out)!=alpha::LogicError::bad_state || costs_out.nutrient!=123 || costs_out.mineral!=124)return 5;
 rules.nutrient_cost_multi=10;
 if(alpha::calculate_human_cost_factors(rules,7,0,0,costs_out)!=alpha::LogicError::bad_state || costs_out.nutrient!=123 || costs_out.mineral!=124)return 6;
 alpha::InitialLandTile tile{false,false,false,0,3};alpha::InitialYieldContext context{0,1,true,{false,false,false}};alpha::ResourceValues value{123,124,125,126};
 if(alpha::initial_land_yield(info,tile,context,value)!=alpha::LogicError::bad_state || value.nutrients!=123 || value.minerals!=124 || value.energy!=125 || value.unused!=126)return 7;
 tile.moisture=2;info.rows[0].energy=-1;
 if(alpha::initial_land_yield(info,tile,context,value)!=alpha::LogicError::bad_state || value.nutrients!=123)return 8;
 alpha::PopulationInput input{1,20,2147483647,10,0,1,2,false,false,true,false};
 alpha::PopulationTransition sentinel{123,124,true,true,true,125},out=sentinel;
 if(alpha::begin_population_growth(input,out)!=alpha::LogicError::overflow || out.population!=123 || out.accumulated!=124 || !out.removed)return 9;
 input.population=0;
 if(alpha::begin_population_growth(input,out)!=alpha::LogicError::bad_state || out.population!=123)return 10;
 alpha::PopulationTransition pending{2,0,true,true,false,-1};
 if(alpha::finish_population_growth(pending,2147483647,1,out)!=alpha::LogicError::overflow || out.population!=123 || out.accumulated!=124)return 11;
 pending.recompute=false;
 if(alpha::finish_population_growth(pending,0,1,out)!=alpha::LogicError::bad_state || out.population!=123)return 12;
 pending.recompute=true;pending.recomputed_accumulation_ceiling=20;
 if(alpha::finish_population_growth(pending,0,1,out)!=alpha::LogicError::bad_state || out.population!=123)return 13;
#ifndef ALPHA_CONTROL_RAW
 printf("costs=%u yields=%u growth=%u digest=%08x rc=0\n",costs,yields,growth,h);
#endif
 return h==0xc2682205u && costs==12348 && yields==6912 && growth==23040?0:20;
}
