/* SPDX-License-Identifier: GPL-3.0-or-later. See production_loans.h. */
#include "production_loans.h"
#include "technology_effects_ranges.h"
namespace alpha { namespace {
bool assign(int32_t&to,int64_t value){if(value<INT32_MIN || value>INT32_MAX)return false;to=(int32_t)value;return true;}
}
LogicError production_loans(const ProductionLoanFacts&in,ProductionLoanResult&out){
 if(detail::effects_range_overlap(&in,sizeof in,&out,sizeof out) || in.faction<0 || in.faction>7 || in.alive_mask>255)return LogicError::bad_state;
 ProductionLoanResult next{};for(unsigned i=0;i<8;++i){next.credits[i]=in.credits[i];next.balance[i]=in.balance[i];}
 const unsigned phase=(unsigned)in.faction;
 for(unsigned i=1;i<8;++i){
  if(i==phase || !(in.alive_mask&(1u<<phase)) || !(in.alive_mask&(1u<<i)) || !next.balance[i] || in.sanctions[i])continue;
  if(in.diplomacy[i]&0x10u){if(!assign(next.balance[i],(int64_t)next.balance[i]+in.payment[i]))return LogicError::overflow;}
  else {
   const int32_t paid=next.credits[phase]<in.payment[i]?next.credits[phase]:in.payment[i];
   if(!assign(next.credits[i],(int64_t)next.credits[i]+paid) ||
      !assign(next.balance[i],(int64_t)next.balance[i]-paid) ||
      !assign(next.credits[phase],(int64_t)next.credits[phase]-paid))return LogicError::overflow;
   if(paid<in.payment[i]){int32_t unpaid;if(!assign(unpaid,(int64_t)in.payment[i]-paid) || !assign(next.balance[i],(int64_t)next.balance[i]+unpaid))return LogicError::overflow;}
  }
 }
 next.energy_before_bases=next.credits[phase];out=next;return LogicError::ok;
}
}
