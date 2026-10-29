/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_inefficiency.h"
#include <stddef.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
bool word(int64_t n){return n>=INT32_MIN&&n<=INT32_MAX;}
}
InefficiencyError energy_inefficiency(const InefficiencyFacts&f,InefficiencyResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return InefficiencyError::bad_buffer;
 if(f.hq_distance<0||f.hq_distance>=999||f.creche>1||f.upkeep_stage>1)return InefficiencyError::bad_state;
 InefficiencyResult next{};for(unsigned j=0;j<9;++j)next.counters[j]=f.counters[j];
 if(f.energy<=0||!f.hq_distance){output=next;return InefficiencyError::ok;}
 const int64_t product=(int64_t)f.energy*f.hq_distance;
 // The original diagnostic and final-loss paths both multiply signed32.
 if(f.upkeep_stage==1&&!word(product))return InefficiencyError::overflow;
 if(f.upkeep_stage==1)for(int32_t j=0;j<9;++j){
  const int32_t ineffective=j-(f.creche?2:0);
  const int64_t increment=ineffective>=8?f.energy:product/(64-8*j+(f.creche?16:0));
  const int64_t total=(int64_t)f.counters[j]+increment;if(!word(total))return InefficiencyError::overflow;
  next.counters[j]=(int32_t)total;
 }
 const int64_t negated=-(int64_t)f.pending_efficiency;
 if(!word(negated)||!word(negated+4))return InefficiencyError::overflow;
 const int64_t ineffective=negated+(f.creche?2:4);
 if(ineffective>=8)next.loss=f.energy;
 else{
  if(!word(product))return InefficiencyError::overflow;
  const int64_t effective=8-ineffective,denominator=effective*8;
  if(!word(effective)||!word(denominator))return InefficiencyError::overflow;
  int64_t loss=product/denominator;if(loss<0)loss=0;if(loss>f.energy)loss=f.energy;next.loss=(int32_t)loss;
 }
 output=next;return InefficiencyError::ok;
}
}
