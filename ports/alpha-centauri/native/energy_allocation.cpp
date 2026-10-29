/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_allocation.h"
#include <stddef.h>
namespace alpha {namespace {
bool word(int64_t n){return n>=INT32_MIN&&n<=INT32_MAX;}
bool add(int32_t&n,int32_t v){const int64_t x=(int64_t)n+v;if(!word(x))return false;n=(int32_t)x;return true;}
int32_t clamp(int32_t n,int32_t lo,int32_t hi){return n<lo?lo:n>hi?hi:n;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
}
InefficiencyError energy_allocation(const EnergyAllocationFacts&f,EnergyAllocationResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return InefficiencyError::bad_buffer;
 if(f.allocation_psych<0||f.allocation_psych>10||f.allocation_labs<0||f.allocation_labs>10||f.allocation_psych+f.allocation_labs>10)return InefficiencyError::bad_state;
 EnergyAllocationResult next{};next.intake=f.intake;next.consumption=f.convoy_from;next.faction_surplus=f.faction_surplus;
 if(!add(next.intake,f.convoy_to))return InefficiencyError::overflow;
 const int64_t first_net=(int64_t)next.intake-next.consumption;if(!word(first_net))return InefficiencyError::overflow;
 InefficiencyFacts loss{};loss.energy=(int32_t)first_net;loss.pending_efficiency=f.pending_efficiency;loss.hq_distance=f.hq_distance;loss.creche=f.creche;loss.upkeep_stage=f.upkeep_stage;for(unsigned j=0;j<9;++j)loss.counters[j]=f.counters[j];
 InefficiencyResult first{};auto error=energy_inefficiency(loss,first);if(error!=InefficiencyError::ok)return error;
 const int64_t initial_surplus=first_net-first.loss;if(!word(initial_surplus))return InefficiencyError::overflow;
 // Source energy-stage faction accumulation precedes the second recomputation.
 if(f.upkeep_stage==1&&!add(next.faction_surplus,clamp((int32_t)initial_surplus,0,99999)))return InefficiencyError::overflow;
 for(int32_t import:f.commerce)if(!add(next.intake,import))return InefficiencyError::overflow;
 const int64_t final_net=(int64_t)next.intake-next.consumption;if(!word(final_net))return InefficiencyError::overflow;
 loss.energy=(int32_t)final_net;for(unsigned j=0;j<9;++j)loss.counters[j]=first.counters[j];InefficiencyResult second{};error=energy_inefficiency(loss,second);if(error!=InefficiencyError::ok)return error;
 next.inefficiency=second.loss;const int64_t surplus=final_net-second.loss;if(!word(surplus))return InefficiencyError::overflow;next.surplus=(int32_t)surplus;for(unsigned j=0;j<9;++j)next.counters[j]=second.counters[j];
 const int32_t total=clamp(next.surplus,0,9999);next.psych=clamp((total*f.allocation_psych+4)/10,0,total);next.economy=clamp((total*(10-f.allocation_labs-f.allocation_psych)+4)/10,0,total-next.psych);next.labs=total-next.psych-next.economy;next.unknown_total=clamp((total*(9-f.allocation_labs-f.allocation_psych)+4)/10,0,total-next.psych);
 int32_t imbalance=f.allocation_psych-10+2*f.allocation_labs;if(imbalance<0)imbalance=-imbalance;imbalance/=2;const int32_t efficient=clamp(f.pending_efficiency,-4,4);const int32_t penalty=(4-efficient)*2*imbalance;
 if(penalty){const bool labs_heavier=10-f.allocation_labs-f.allocation_psych<f.allocation_labs;const int32_t labs_penalty=clamp(penalty*(labs_heavier?1:2),0,labs_heavier?40:80);const int32_t economy_penalty=clamp(penalty*(labs_heavier?2:1),0,labs_heavier?80:40);next.labs=((100-labs_penalty)*next.labs+50)/100;next.economy=((100-economy_penalty)*next.economy+50)/100;}
 output=next;return InefficiencyError::ok;
}
}
