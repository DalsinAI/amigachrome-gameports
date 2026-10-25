/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "exploration.h"
namespace alpha {
LogicError exploration_step(const MovementMap& map,const UnitDefinitions& defs,
 const MovementContext& ctx,int32_t direction,ExplorationUnit& unit,StepResult& output) {
  if(direction<0 || direction>=8 || ctx.faction<1 || ctx.xenoempathy_dome || ctx.ignore_rough)
    return LogicError::bad_state;
  PredefinedPrototype proto;
  auto error=derive_predefined_prototype(defs,unit.prototype,ctx.road_rate,proto);
  if(error!=LogicError::ok)return error;
  if(defs.chassis[proto.chassis].triad!=0 || proto.speed_points>255 ||
     unit.budget.speed!=proto.speed_points)return LogicError::bad_state;
  // Admit only known inert terrain. Any event/ownership/base bit is unsupported.
  const uint32_t supported=4u|8u|0x20u|0x80u|0x200000u;
  const size_t need=(size_t)(map.geometry.width/2)*(size_t)map.geometry.height;
  size_t source;
  if(!map.tiles || map.count!=need || movement_tile_index(map.geometry,unit.x,unit.y,source)!=LogicError::ok)
    return LogicError::bad_state;
  for(size_t i=0;i<map.count;++i)if(map.tiles[i].items&~supported)return LogicError::bad_state;
  if(map.tiles[source].altitude<0x60)return LogicError::bad_state;
  const int32_t dx[]={1,2,1,0,-1,-2,-1,0},dy[]={-1,0,1,2,1,0,-1,-2};
  int32_t tx;
  if(movement_wrap_x(map.geometry,unit.x+dx[direction],tx)!=LogicError::ok)return LogicError::bad_state;
  const int32_t ty=unit.y+dy[direction];size_t dest;
  if(movement_tile_index(map.geometry,tx,ty,dest)!=LogicError::ok || map.tiles[dest].altitude<0x60) {
    output={StepOutcome::blocked,0};return LogicError::ok;
  }
  uint32_t cost;
  error=adjacent_movement_cost(map,defs,unit.prototype,ctx,unit.x,unit.y,tx,ty,cost);
  if(error!=LogicError::ok)return error;
  MoveBudgetInput input{cost,(uint32_t)ctx.road_rate,true,(map.tiles[dest].items&0x20u)!=0,
    defs.weapons[proto.weapon].offense<0,proto.plan==9 || proto.plan==12,false,true,true,true};
  MoveBudgetResult budget;
  error=resolve_move_budget(unit.budget,input,budget);
  if(error!=LogicError::ok)return error;
  auto updated=unit;updated.budget=budget.budget;
  StepOutcome outcome=StepOutcome::exhausted;
  if(budget.outcome==MoveBudgetOutcome::move){updated.x=tx;updated.y=ty;outcome=StepOutcome::moved;}
  else if(budget.outcome==MoveBudgetOutcome::fractional_failure)outcome=StepOutcome::fractional_failure;
  unit=updated;output={outcome,cost};return LogicError::ok;
}
LogicError exploration_next_movement_turn(ExplorationUnit& unit,uint32_t& turn) {
  if(unit.budget.speed>255 || unit.budget.spent>255)return LogicError::bad_state;
  if(turn==0xffffffffu)return LogicError::overflow;
  // The qualified movement-only part of repair_phase: other repair/upkeep
  // systems are deliberately absent; the fractional retry bit is not cleared.
  unit.budget.spent=0;++turn;return LogicError::ok;
}
}
