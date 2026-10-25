/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "exploration.h"
#include <stdio.h>
#include <initializer_list>
#include <string.h>
#define ALPHA_CONTROL_NO_MAIN
#include "prototype_control.cpp"
static alpha::UnitDefinitions d;
static alpha::MovementTile tiles[32];
int main(){
  prototype_control::fixture(d,true);d.units[2].chassis=0;d.units[2].weapon=0;d.units[2].abilities=0;
  d.chassis[0].triad=0;d.chassis[0].speed=1;
  for(auto& t:tiles)t={4,0x60,0};
  alpha::PredefinedPrototype proto;if(alpha::derive_predefined_prototype(d,2,3,proto)!=alpha::LogicError::ok || proto.speed_points!=3)return 1;
  alpha::MovementMap map{{8,8,false},tiles,32};alpha::MovementContext ctx{1,0,3,false,false};
  alpha::ExplorationUnit u{2,0,2,{3,0,0,0x12345678u}};alpha::StepResult r;uint32_t turn=0;
  for(int x:{2,4,6}){if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || r.outcome!=alpha::StepOutcome::moved || u.x!=x || r.cost!=1)return 2;}
  if(u.budget.spent!=3 || alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || r.outcome!=alpha::StepOutcome::exhausted || u.x!=6)return 3;
  if(alpha::exploration_next_movement_turn(u,turn)!=alpha::LogicError::ok || turn!=1 || u.budget.spent)return 4;
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || u.x!=0 || r.outcome!=alpha::StepOutcome::moved)return 5;
  tiles[10].altitude=0x40; // (4,2)
  auto before=u;
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || u.x!=2)return 6;
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || r.outcome!=alpha::StepOutcome::blocked || u.x!=2 || u.budget.spent!=2)return 7;
  tiles[10]={0,0x60,0x80};u={2,2,2,{3,0,0,1}};
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || r.outcome!=alpha::StepOutcome::moved || u.x!=4 || u.budget.spent!=6)return 8;
  // Budget may overrun speed; subsequent move must be exhausted, not wrap.
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok || r.outcome!=alpha::StepOutcome::exhausted)return 9;
  unsigned failed=0,succeeded=0;
  tiles[10]={0x20,0x60,0};
  for(uint32_t seed=0;seed<128;++seed){u={2,2,2,{3,2,0,seed}};
    if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::ok)return 10;
    if(r.outcome==alpha::StepOutcome::fractional_failure){++failed;if(u.x!=2 || u.budget.spent!=3 || !(u.budget.state&0x8000))return 11;
      if(alpha::exploration_next_movement_turn(u,turn)!=alpha::LogicError::ok || !(u.budget.state&0x8000))return 12;
    }else if(r.outcome==alpha::StepOutcome::moved){++succeeded;if(u.x!=4 || u.budget.spent!=11 || (u.budget.state&0x8000))return 13;}
    else return 14;
  }
  if(!failed || !succeeded)return 15;
  u={2,2,2,{3,0,0,1}};before=u;tiles[0].items=1; // a base would need unsupported systems
  if(alpha::exploration_step(map,d,ctx,1,u,r)!=alpha::LogicError::bad_state || memcmp(&u,&before,sizeof(u)))return 16;
  tiles[0].items=4;
  if(alpha::exploration_step(map,d,ctx,8,u,r)!=alpha::LogicError::bad_state || memcmp(&u,&before,sizeof(u)))return 17;
  turn=0xffffffffu;
  if(alpha::exploration_next_movement_turn(u,turn)!=alpha::LogicError::overflow || memcmp(&u,&before,sizeof(u)))return 18;
 // Friendly bases must be explicitly owned; arbitrary base bits remain rejected.
  tiles[10]={1,0x60,0};u={2,2,2,{3,0,0,1}};size_t base=10;
  if(alpha::exploration_step(map,d,ctx,1,u,r,nullptr,1)!=alpha::LogicError::bad_state || u.x!=2)return 20;
  if(alpha::exploration_step(map,d,ctx,1,u,r,&base,1)!=alpha::LogicError::ok || u.x!=4 || r.cost!=1 || u.budget.spent!=1)return 21;
#ifndef ALPHA_CONTROL_RAW
  printf("scenario=128 road=3 wrap=1 ocean=1 rough=1 exhausted=2 invalid=3 fractional_failed=%u fractional_moved=%u rc=0\n",failed,succeeded);
#endif
  return failed==112 && succeeded==16?0:19;
}
