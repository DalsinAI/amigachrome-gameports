/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in upkeep_base_energy.h. */
#include "upkeep_base_energy.h"
namespace alpha {namespace {
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool overlap(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
}
EnergyError base_energy_accounting(const EnergyFacts&f,EnergyResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return EnergyError::bad_buffer;
 if(f.fresh_intake<0||f.convoy_to<0||f.commerce<0||f.energy_grid<0||f.convoy_from<0||f.headquarters_distance< -1||f.upkeep_stage<0||f.upkeep_stage>2)return EnergyError::bad_state;
 EnergyResult r{};for(unsigned j=0;j<9;++j)r.social_efficiency_after[j]=f.social_efficiency_before[j];
 if(!put((int64_t)f.fresh_intake+f.convoy_to,r.intake)||!put((int64_t)r.intake+f.commerce,r.intake)||!put((int64_t)r.intake+f.energy_grid,r.intake))return EnergyError::overflow;
 r.consumption=f.convoy_from;int32_t energy;
 if(!put((int64_t)r.intake-r.consumption,energy))return EnergyError::overflow;
 const int32_t dist=f.headquarters_distance<0||f.headquarters_distance>=999?16:f.headquarters_distance;
 if(energy>0&&dist){
  int32_t product=0;bool product_ready=false;
  if(f.upkeep_stage==1){
   if(!put((int64_t)energy*dist,product))return EnergyError::overflow;
   product_ready=true;
   for(int j=0;j<9;++j){const int ineff=j-(f.physical_creche?2:0);const int denom=(8-j+(f.physical_creche?2:0))*8;
    const int32_t change=ineff>=8?energy:product/denom;
    if(!put((int64_t)r.social_efficiency_after[j]+change,r.social_efficiency_after[j]))return EnergyError::overflow;
   }
  }
  int32_t ineff;if(!put(4LL-f.pending_efficiency,ineff)||!put((int64_t)ineff-(f.physical_creche?2:0),ineff))return EnergyError::overflow;
  if(ineff>=8)r.inefficiency=energy;
  else {int32_t factor,denom;if(!put(8LL-ineff,factor)||!put((int64_t)factor*8,denom))return EnergyError::overflow;
   if(!product_ready&&!put((int64_t)energy*dist,product))return EnergyError::overflow;
   const int32_t value=product/denom;r.inefficiency=value<0?0:value>energy?energy:value;
  }
 }
 if(!put((int64_t)energy-r.inefficiency,r.surplus))return EnergyError::overflow;
 output=r;return EnergyError::ok;
}
}
