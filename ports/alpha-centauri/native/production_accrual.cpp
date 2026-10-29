/* SPDX-License-Identifier: GPL-3.0-or-later. See production_accrual.h. */
#include "production_accrual.h"
namespace alpha { namespace {
bool word(int64_t n){return n>=(-2147483647LL-1) && n<=2147483647LL;}
}
LogicError production_accrual(const ProductionAccrualInput& in,ProductionAccrualResult& output){
 if(in.variant!=Variant::base && in.variant!=Variant::alien_crossfire)return LogicError::bad_state;
 const int64_t doubled=(int64_t)in.mineral_surplus*2;
 const int64_t peak=(int64_t)in.mineral_intake_2+doubled;
 if(!word(doubled) || !word(peak))return LogicError::overflow;
 ProductionAccrualResult next{in.state_flags&~0x800000u,
  in.best_mineral_output>(int32_t)peak?in.best_mineral_output:(int32_t)peak,
  in.minerals_accumulated,in.nutrients_accumulated<0,false};
 const int32_t stockpile=in.variant==Variant::base?-38:-69;
 if(!next.negative_food_abort && in.mineral_surplus>=0 && in.item!=stockpile && !(next.state_flags&2u)){
  const int64_t accumulated=(int64_t)in.minerals_accumulated+in.mineral_surplus;
  if(!word(accumulated))return LogicError::overflow;
  next.minerals_accumulated=(int32_t)accumulated;next.accrual_branch=true;
 }
 output=next;return LogicError::ok;
}
}
