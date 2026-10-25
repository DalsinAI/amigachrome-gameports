/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in move_budget.h. */
#include "move_budget.h"
namespace alpha {
uint32_t integer_random(uint32_t& seed,uint32_t minimum,uint32_t maximum) {
  if(minimum>maximum){uint32_t swap=minimum;minimum=maximum;maximum=swap;}
  seed=seed*0x19660du+0x3c6ef35fu;
  return (((maximum-minimum)*(uint16_t)seed)>>16)+minimum;
}
LogicError resolve_move_budget(const MoveBudget& before,const MoveBudgetInput& in,
                               MoveBudgetResult& output) {
  if(before.speed>255 || before.spent>255 || in.cost>500 || in.road_rate<1 || in.road_rate>100)
    return LogicError::bad_state;
  MoveBudgetResult result{before,MoveBudgetOutcome::exhausted,false,0};
  if(before.spent>=before.speed){output=result;return LogicError::ok;}
  const uint32_t remaining=before.speed-before.spent;
  if(in.cost>remaining && in.land &&
      (remaining<in.road_rate || (in.destination_fungus && !in.native_unit &&
       !in.former_or_artifact && !in.destination_occupied)) && (in.attack_check || !in.human)) {
    result.random_value=integer_random(result.budget.random_seed,0,in.cost);
    result.drew_random=true;
    const uint32_t allowance=remaining+((before.state&0x8000u)?in.road_rate:0);
    if(result.random_value>=allowance) {
      result.budget.spent=before.speed;
      result.budget.state|=0x8000u;
      result.outcome=MoveBudgetOutcome::fractional_failure;
      output=result;return LogicError::ok;
    }
  }
  if(!in.execute){result.outcome=MoveBudgetOutcome::preview;output=result;return LogicError::ok;}
  if(in.cost>255-before.spent)return LogicError::overflow;
  result.budget.spent+=in.cost;
  result.budget.state&=~(0x8000u|0x40u);
  result.outcome=MoveBudgetOutcome::move;
  output=result;return LogicError::ok;
}
}
