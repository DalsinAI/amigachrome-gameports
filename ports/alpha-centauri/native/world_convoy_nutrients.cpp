/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_convoy_nutrients.h"
#include "movement.h"
#include "world_fresh_tile_yields.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool alias(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,const StableIntakeStage&s,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&t,sizeof t)||overlap(out,n,&b,sizeof b)||overlap(out,n,&r,sizeof r)||overlap(out,n,&s,sizeof s))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool add(int32_t&v,int32_t x){const int64_t n=(int64_t)v+x;if(n<(-2147483647LL-1)||n>2147483647LL)return false;v=(int32_t)n;return true;}
}
StableIntakeError world_convoy_flows(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const StableIntakeStage&s,WorldConvoyFlows&output){
 if(alias(w,t,b,r,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 if(!w.loaded()||id>2147483647u||(s.variant!=Variant::base&&s.variant!=Variant::alien_crossfire)||(s.variant==Variant::base&&s.expansion_enabled))return StableIntakeError::bad_state;
 if(s.worker.upkeep_state!=0||s.worker.editor_active)return StableIntakeError::unsupported_stage;
 const auto*base=w.base(id);if(!base||base->faction_id<0||base->faction_id>7)return StableIntakeError::bad_state;
 const auto&g=w.game();const auto&m=g.map.info;
 const MapGeometry geometry{(int32_t)m.width,(int32_t)m.height,m.flat};
 WorldTileYieldStage ts{s.variant,s.expansion_enabled,false,s.control_upkeep,(int32_t)id,s.terraform_reduction_before,s.base_energy_before};
 // Validate the complete context even for an empty or non-convoy world.
 TileYieldContext check{};
 const auto initial=world_fresh_tile_yield_context(w,t,b,r,id,base->faction_id,base->x,base->y,ts,check);
 if(initial==TileYieldError::overflow)return StableIntakeError::overflow;
 if(initial!=TileYieldError::ok)return StableIntakeError::bad_state;
 WorldConvoyFlows next{};next.terraform_reduction_after=s.terraform_reduction_before;next.base_energy_after=s.base_energy_before;
 for(size_t i=0;i<g.vehicle_count;++i){
  const auto*v=w.vehicle(i);size_t tile_id;
  if(!v||v->faction_id>7||v->unit_id<0||v->unit_id>=512||v->home_base_id< -1||v->home_base_id>=(int32_t)g.base_count||movement_tile_index(geometry,v->x,v->y,tile_id)!=LogicError::ok)return StableIntakeError::bad_state;
  if(v->faction_id!=base->faction_id)continue;
  const auto*p=w.prototype((size_t)v->unit_id);if(!p)return StableIntakeError::bad_state;
  if(p->plan!=10||v->order!=3)continue;
  const unsigned channel=v->order_auto_type;if(channel>=4)return StableIntakeError::bad_state;
  if(v->home_base_id==(int32_t)id){
   const auto*tile=w.tile(tile_id);if(!tile)return StableIntakeError::bad_state;
   if((tile->items&1u)&&(tile->val2&15u)<8){if(!add(next.from[channel],1))return StableIntakeError::overflow;continue;}
   if(!add(next.harvesters,1))return StableIntakeError::overflow;
   if(channel==3){next.terraform_reduction_after=0;continue;}
   ts.terraform_reduction_before=next.terraform_reduction_after;ts.base_energy_before=next.base_energy_after;
   TileYieldResult value{};const auto e=world_fresh_tile_yields(w,t,b,r,id,base->faction_id,v->x,v->y,ts,value);
   if(e==TileYieldError::overflow)return StableIntakeError::overflow;
   if(e!=TileYieldError::ok)return StableIntakeError::bad_state;
   const int32_t yields[]={value.nutrient,value.mineral,value.energy};
   if(!add(next.to[channel],yields[channel])||!add(next.terraform_reduction_after,value.reduction_by_resource[channel]))return StableIntakeError::overflow;
   // Only energy_yield can write BaseEnergy, and exterior harvests are not base squares.
   if(channel==2)next.base_energy_after=value.base_energy;
  }else if(v->home_base_id>=0&&v->x==base->x&&v->y==base->y){if(!add(next.to[channel],1))return StableIntakeError::overflow;}
 }
 output=next;return StableIntakeError::ok;
}
StableIntakeError world_convoy_nutrients(const OriginalWorld&w,const Rules&t,const BasicRules&b,const ResourceInfo&r,size_t id,const StableIntakeStage&s,ConvoyNutrientResult&output){
 if(alias(w,t,b,r,s,&output,sizeof output))return StableIntakeError::bad_buffer;
 ConvoyNutrientResult next{};const auto flow=world_convoy_flows(w,t,b,r,id,s,next.convoys);if(flow!=StableIntakeError::ok)return flow;
 FreshNutrientStage stage{s,next.convoys.to[0],next.convoys.from[0]};stage.intake.terraform_reduction_before=next.convoys.terraform_reduction_after;stage.intake.base_energy_before=next.convoys.base_energy_after;
 const auto fresh=world_fresh_nutrients(w,t,b,r,id,stage,next.fresh);if(fresh!=StableIntakeError::ok)return fresh;
 output=next;return StableIntakeError::ok;
}
}
