/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portions adapted from Thinker, MIT; Copyright (c) Thinker Mod authors.
 * See THINKER-LICENSE.txt and pin in world_unit_stats.h.
 * Restricted integration of qualified movement price/speed and source-backed
 * veh_lift/drop. Source rules: Thinker order_veh and OpenSMACX veh.cpp, pins in
 * world_unit_stats.h, move_budget.h and vehicle_stack.h.
 */
#include "world_actions.h"
#include "move_budget.h"
namespace alpha {
namespace {
constexpr int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};
// Original non-rare pod placement. Negative x-difference uses arithmetic
// shift in the first expression and truncating division in the second.
bool has_pod(const OriginalMapTile& tile,int x,int y,uint32_t seed,int32_t rules){
 const uint32_t bits=tile.items;
 if(bits&(0x100000u|0x2000u))return false;
 if(rules&0x2000)return (bits&(0x4000000u|0x8000000u))!=0;
 if(bits&0x10000000u)return true;
 if(!seed)return false;
 const int avg=(x+y)/2,diff=x-avg;
 const int shifted=diff>=0?diff/4:-((-diff+3)/4);
 const uint32_t cmp=(uint32_t)(avg&3)+4u*((uint32_t)diff&3u);
 const uint32_t land=(seed-5u*(uint32_t)(avg/4)-3u*(uint32_t)shifted)&15u;
 if((tile.climate&0xe0u)>=0x60u && cmp==land)return true;
 return cmp==((11u*(uint32_t)(avg/4)+61u*(uint32_t)(diff/4)+seed+8u)&31u);
}
bool nearby(const MapGeometry& g,int ax,int ay,int bx,int by){
 int32_t xdistance=0;
 return movement_distance_x(g,ax,bx,xdistance)==LogicError::ok &&
        xdistance<=2 && (ay>by?ay-by:by-ay)<=2;
}
}
StepResult WorldActions::clear_land_step(OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,
 size_t id,int32_t player,int32_t road,int32_t x,int32_t y){
 if(!w.loaded() || player<1 || player>7 || road<1 || road>100 || tech.count!=89)return {StepStatus::bad_state,0};
 auto* v=id<w.view_.vehicle_count?w.vehicles_+id:nullptr;
 if(!v || v->unit_id<0 || v->unit_id>=512)return {StepStatus::bad_state,0};
 if(player!=w.game().player || v->faction_id!=player)return {StepStatus::not_owner,0};
 // Full SMAX abilities and turn dispatch are separate integration tasks. Do
 // not accidentally apply old base component IDs to expansion data.
 if(w.game().data_version>=11 || d.weapon_count!=21)return {StepStatus::unsupported,0};
 const auto& p=w.prototypes_[v->unit_id];
 uint32_t speed=0;
 if(world_vehicle_speed(w,d,tech,id,road,false,speed)!=LogicError::ok)return {StepStatus::bad_state,0};
 if(d.chassis[p.chassis_id].triad!=0 || p.ability_flags || p.plan<0 || p.plan>3 ||
    d.weapons[p.weapon_id].offense<0 || v->state&0x0b004201u || v->order || v->waypoint_count)
  return {StepStatus::unsupported,0};
 const auto& definition=w.definitions_[player];
 if(definition.rule_flags&0x80u || definition.faction_bonus_count<0 || definition.faction_bonus_count>8)
  return {StepStatus::unsupported,0};
 // Ability grants belong to the saved design's owner, which can differ from
 // the vehicle owner. Full FREEABIL speed integration is still pending.
 const int owners[2]={player,v->unit_id/64};
 for(int owner:owners){const auto& rules=w.definitions_[owner];
  if(rules.faction_bonus_count<0 || rules.faction_bonus_count>8)return {StepStatus::unsupported,0};
  for(int i=0;i<rules.faction_bonus_count;++i)if(rules.faction_bonus_id[i]==15)
   return {StepStatus::unsupported,0};
 }
 MapGeometry geometry{(int32_t)w.view_.map.info.width,(int32_t)w.view_.map.info.height,w.view_.map.info.flat};
 size_t from,to;int direction=-1;
 if(movement_tile_index(geometry,v->x,v->y,from)!=LogicError::ok ||
    movement_tile_index(geometry,x,y,to)!=LogicError::ok)return {StepStatus::bad_state,0};
 for(int i=0;i<8;++i){int32_t wrapped;
  if(movement_wrap_x(geometry,v->x+dx[i],wrapped)==LogicError::ok && wrapped==x && v->y+dy[i]==y){direction=i;break;}}
 if(direction<0)return {StepStatus::bad_state,0};
 const auto& a=w.tiles_[from];const auto& b=w.tiles_[to];
 if((a.climate&0xe0u)<0x60u || (b.climate&0xe0u)<0x60u ||
    (b.items&(0x20u|0x2000u)) || has_pod(b,x,y,w.view_.map.info.random_seed,w.view_.preferences.GameRules) ||
    (a.owner>=0 && a.owner!=player) || (b.owner>=0 && b.owner!=player))return {StepStatus::unsupported,0};
 // Already-seen local neighborhoods only: movement revelation, contact and
 // tutorial event dispatch are deliberately not approximated here.
 for(int point=0;point<2;++point){int cx=point?x:v->x,cy=point?y:v->y;
  for(int i=-1;i<8;++i){int32_t px=cx;int py=cy;
   if(i>=0){if(movement_wrap_x(geometry,cx+dx[i],px)!=LogicError::ok)return {StepStatus::bad_state,0};py+=dy[i];}
   size_t tile;if(movement_tile_index(geometry,px,py,tile)!=LogicError::ok)continue;
   if(!(w.tiles_[tile].visibility&(1u<<player)))return {StepStatus::unsupported,0};
  }
 }
 // Refuse foreign presence altogether rather than silently inventing pact,
 // combat, visibility/contact and ZOC outcomes. This is a supported subset,
 // not a claim that all such moves would be illegal in the original game.
 for(size_t i=0;i<w.view_.vehicle_count;++i){const auto& other=w.vehicles_[i];
  if(other.faction_id!=player && (nearby(geometry,v->x,v->y,other.x,other.y) || nearby(geometry,x,y,other.x,other.y)))
   return {StepStatus::unsupported,0};
 }
 for(size_t i=0;i<w.view_.base_count;++i){const auto& base=w.bases_[i];
  if(base.faction_id!=player && (nearby(geometry,v->x,v->y,base.x,base.y) || nearby(geometry,x,y,base.x,base.y)))
   return {StepStatus::unsupported,0};
 }
 uint32_t price=0;
 if(world_movement_price(w,d,tech,id,road,x,y,false,price)!=LogicError::ok)return {StepStatus::bad_state,0};
 if(v->moves_spent>=speed)return {StepStatus::exhausted,price};
 if(price>speed-v->moves_spent)return {StepStatus::unsupported,price}; // Fractional attempts deferred.
 MoveBudgetResult budget;
 const MoveBudget before{speed,v->moves_spent,v->state|4u,w.random_seed_};
 const MoveBudgetInput input{price,(uint32_t)road,true,false,false,false,(b.items&2u)!=0,true,true,true};
 if(resolve_move_budget(before,input,budget)!=LogicError::ok || budget.outcome!=MoveBudgetOutcome::move)
  return {StepStatus::bad_state,price};
 VehicleStackStorage storage{geometry.width,geometry.height,w.tiles_,w.view_.map.info.tile_count,w.vehicles_,w.view_.vehicle_count};
 const auto error=relocate_vehicle_storage(storage,id,x,y,0);
 if(error!=StackError::ok)return {error==StackError::out_of_memory?StepStatus::out_of_memory:StepStatus::bad_state,price};
 // No possible failure after the first storage write. These are the source's
 // movement-attempt/price/facing fields, not an invented turn or RNG draw.
 v->moves_spent=(uint8_t)budget.budget.spent;v->state=budget.budget.state;
 v->rotate_angle=(int8_t)direction;w.random_seed_=budget.budget.random_seed;
 return {StepStatus::moved,price};
}
StepResult WorldActions::clear_sea_step(OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,
 size_t id,int32_t player,int32_t road,int32_t x,int32_t y){
 if(!w.loaded() || player<1 || player>7 || road<1 || road>100 || tech.count!=89)return {StepStatus::bad_state,0};
 auto* v=id<w.view_.vehicle_count?w.vehicles_+id:nullptr;
 if(!v || v->unit_id<0 || v->unit_id>=512)return {StepStatus::bad_state,0};
 if(player!=w.game().player || v->faction_id!=player)return {StepStatus::not_owner,0};
 if(w.game().data_version>=11 || d.weapon_count!=21 ||
    !(w.game().preferences.FactionStatus[0]&(1u<<player)) ||
    w.game().preferences.CurrentFaction!=player)return {StepStatus::unsupported,0};
 const auto& p=w.prototypes_[v->unit_id];uint32_t speed=0;
 if(world_vehicle_speed(w,d,tech,id,road,false,speed)!=LogicError::ok)return {StepStatus::bad_state,0};
 const bool transport=p.plan==7 && d.weapons[p.weapon_id].mode==7 && p.carry_capacity>0;
 const bool combat=p.plan>=0 && p.plan<=3 && d.weapons[p.weapon_id].mode==0 && !p.carry_capacity;
 if(d.chassis[p.chassis_id].triad!=1 || p.ability_flags || (!transport && !combat) ||
    d.weapons[p.weapon_id].offense<0 || (v->state&0x0b004201u) || v->order || v->waypoint_count)
  return {StepStatus::unsupported,0};
 if(w.definitions_[player].rule_flags&0x80u)return {StepStatus::unsupported,0};
 const int owners[2]={player,v->unit_id/64};
 for(int owner:owners){const auto& rules=w.definitions_[owner];
  if(rules.faction_bonus_count<0 || rules.faction_bonus_count>8)return {StepStatus::unsupported,0};
  for(int i=0;i<rules.faction_bonus_count;++i)if(rules.faction_bonus_id[i]==15)return {StepStatus::unsupported,0};
 }
 MapGeometry g{(int32_t)w.view_.map.info.width,(int32_t)w.view_.map.info.height,w.view_.map.info.flat};
 size_t from,to;int direction=-1;
 if(movement_tile_index(g,v->x,v->y,from)!=LogicError::ok || movement_tile_index(g,x,y,to)!=LogicError::ok)
  return {StepStatus::bad_state,0};
 for(int i=0;i<8;++i){int32_t wrapped;
  if(movement_wrap_x(g,v->x+dx[i],wrapped)==LogicError::ok && wrapped==x && v->y+dy[i]==y){direction=i;break;}}
 if(direction<0)return {StepStatus::bad_state,0};
 const auto& a=w.tiles_[from];const auto& b=w.tiles_[to];
 bool at_base[2]{};
 for(int point=0;point<2;++point){const int cx=point?x:v->x,cy=point?y:v->y;const auto& tile=point?b:a;
  int bases=0;for(size_t i=0;i<w.view_.base_count;++i){const auto& base=w.bases_[i];
   if(base.x==cx && base.y==cy){if(base.faction_id!=player)return {StepStatus::unsupported,0};++bases;}}
  if(bases>1 || ((tile.items&1u)!=0)!=(bases==1))return {StepStatus::bad_state,0};
  at_base[point]=bases==1;
  if((tile.climate&0xe0u)>=0x60u){
   if(!at_base[point])return {StepStatus::unsupported,0};
   bool coastal=false;for(int i=0;i<8;++i){int32_t px;size_t tile_index;
    if(movement_wrap_x(g,cx+dx[i],px)==LogicError::ok &&
       movement_tile_index(g,px,cy+dy[i],tile_index)==LogicError::ok &&
       (w.tiles_[tile_index].climate&0xe0u)<0x60u)coastal=true;}
   if(!coastal)return {StepStatus::unsupported,0};
  }
  if((tile.items&(0x20u|0x2000u)) || (tile.owner>=0 && tile.owner!=player))return {StepStatus::unsupported,0};
  for(int i=-1;i<8;++i){int32_t px=cx;int py=cy;
   if(i>=0){if(movement_wrap_x(g,cx+dx[i],px)!=LogicError::ok)return {StepStatus::bad_state,0};py+=dy[i];}
   size_t ti;if(movement_tile_index(g,px,py,ti)!=LogicError::ok)continue;
   if(!(w.tiles_[ti].visibility&(1u<<player)))return {StepStatus::unsupported,0};
  }
 }
 // Water/base edges only; this does not implement land-base canal chains.
 if((a.climate&0xe0u)>=0x60u && (b.climate&0xe0u)>=0x60u)return {StepStatus::unsupported,0};
 if(has_pod(b,x,y,w.view_.map.info.random_seed,w.view_.preferences.GameRules))return {StepStatus::unsupported,0};
 for(size_t i=0;i<w.view_.vehicle_count;++i){const auto& other=w.vehicles_[i];
  if(other.faction_id!=player && (nearby(g,v->x,v->y,other.x,other.y) || nearby(g,x,y,other.x,other.y)))
   return {StepStatus::unsupported,0};
  if(i==id)continue;
  // Source stack_veh/set_board_to encode a boarder using order1 and the
  // transport ID in waypoint_x[0]; VSTATE_IN_TRANSPORT is state bit0.
  // veh_cargo reports CAPACITY, not passengers. Never infer emptiness from it.
  if((other.order==1 || (other.state&1u)) && other.waypoint_x[0]==(int32_t)id)
   return {StepStatus::unsupported,0};
  const bool source=other.x==v->x && other.y==v->y,destination=other.x==x && other.y==y;
  if(!source && !destination)continue;
  if((other.state&1u) || other.order==1)return {StepStatus::unsupported,0};
  if(other.unit_id<0 || other.unit_id>=512 || w.prototypes_[other.unit_id].chassis_id>=9)
   return {StepStatus::bad_state,0};
  // At open sea the source auto-loads co-located land passengers. Defer that
  // entire mixed stack. Ordinary unembarked base garrisons may remain behind;
  // human ships with ON_ALERT are already excluded above.
  if(d.chassis[w.prototypes_[other.unit_id].chassis_id].triad!=1 &&
     ((source && !at_base[0]) || (destination && !at_base[1])))return {StepStatus::unsupported,0};
 }
 for(size_t i=0;i<w.view_.base_count;++i){const auto& base=w.bases_[i];
  if(base.faction_id!=player && (nearby(g,v->x,v->y,base.x,base.y) || nearby(g,x,y,base.x,base.y)))
   return {StepStatus::unsupported,0};}
 uint32_t price=0;if(world_movement_price(w,d,tech,id,road,x,y,false,price)!=LogicError::ok)return {StepStatus::bad_state,0};
 if(v->moves_spent>=speed)return {StepStatus::exhausted,price};
 if(price>speed-v->moves_spent)return {StepStatus::unsupported,price};
 MoveBudgetResult budget;
 const MoveBudget before{speed,v->moves_spent,v->state|4u,w.random_seed_};
 const MoveBudgetInput input{price,(uint32_t)road,false,false,false,false,(b.items&2u)!=0,true,true,true};
 if(resolve_move_budget(before,input,budget)!=LogicError::ok || budget.outcome!=MoveBudgetOutcome::move)
  return {StepStatus::bad_state,price};
 VehicleStackStorage storage{g.width,g.height,w.tiles_,w.view_.map.info.tile_count,w.vehicles_,w.view_.vehicle_count};
 const auto error=relocate_vehicle_storage(storage,id,x,y,1);
 if(error!=StackError::ok)return {error==StackError::out_of_memory?StepStatus::out_of_memory:StepStatus::bad_state,price};
 v->moves_spent=(uint8_t)budget.budget.spent;v->state=budget.budget.state;
 v->rotate_angle=(int8_t)direction;w.random_seed_=budget.budget.random_seed;
 return {StepStatus::moved,price};
}
const char* step_status_name(StepStatus s){switch(s){
 case StepStatus::moved:return "moved";case StepStatus::exhausted:return "exhausted";
 case StepStatus::not_owner:return "not_owner";case StepStatus::unsupported:return "unsupported";
 case StepStatus::bad_state:return "bad_state";case StepStatus::out_of_memory:return "out_of_memory";
 }return "unknown";}
}
