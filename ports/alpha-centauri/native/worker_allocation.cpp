/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in worker_allocation.h. */
#include "worker_allocation.h"
#include <stddef.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
bool put(int64_t x,int32_t&out){if(x<(-2147483647LL-1)||x>2147483647LL)return false;out=(int32_t)x;return true;}
int64_t maximum(int64_t a,int64_t b){return a>b?a:b;}
unsigned bits(uint32_t x){unsigned n=0;while(x){x&=x-1;++n;}return n;}
}
UpkeepWorkerAllocationError worker_allocation_candidate(const WorkerAllocationFacts&f,WorkerAllocationResult&out){
 if(overlap(&f,sizeof f,&out,sizeof out))return UpkeepWorkerAllocationError::bad_buffer;
 if(!(f.available&1u)||(f.available&~0x1fffffu)||f.population<0||f.population>127||f.specialists<0||f.specialists>f.population||f.efficiency<0||f.efficiency>16||f.psych_allocation<0||f.psych_allocation>10||f.nutrient_threshold<0)return UpkeepWorkerAllocationError::bad_state;
 for(unsigned i=0;i<21;++i)for(unsigned k=0;k<3;++k)if(f.yields[i][k]<0)return UpkeepWorkerAllocationError::bad_state;
 WorkerAllocationResult r{};r.worked=(f.worked&f.available)|1u;r.specialists=f.specialists;
 int32_t workers=f.population+1-(int32_t)bits(r.worked)-r.specialists;
 if(workers<0||!f.worked){r.worked=1;r.specialists=0;workers=f.population;r.reset_overassigned=true;}
 r.nutrient_after=f.nutrient_before;r.mineral_after=f.mineral_before;r.energy_after=f.energy_before;
 for(unsigned i=0;i<21;++i)if(r.worked&(1u<<i)){
  if(!put((int64_t)r.nutrient_after+f.yields[i][0],r.nutrient_after)||!put((int64_t)r.mineral_after+f.yields[i][1],r.mineral_after)||!put((int64_t)r.energy_after+f.yields[i][2],r.energy_after))return UpkeepWorkerAllocationError::overflow;
 }
 while(workers>0){
  int choice=-1;int32_t best=0;
  for(unsigned i=1;i<21;++i){if(!(f.available&(1u<<i))||(r.worked&(1u<<i)))continue;
   const int64_t N=r.nutrient_after,M=r.mineral_after,E=r.energy_after;
   int32_t psych_energy=0;
   if(f.can_riot&&f.efficiency>=12&&f.psych_allocation>0&&!put(E*f.psych_allocation+4,psych_energy))return UpkeepWorkerAllocationError::overflow;
   const int energy_value=f.can_riot&&f.efficiency>=12&&f.psych_allocation>0&&psych_energy/10<2?6:4;
   const int64_t nutrient_value=f.can_grow?maximum(4+4*(N<0)+4*(N<f.nutrient_threshold)+(f.population==1&&N<=0&&M>0?6:0)+maximum(0,5-f.population)-maximum(0,N-1)/4,2):(N<0?8:2);
   const int64_t mineral_value=5+5*(M<0)+4*(M<2);
   int32_t score;
   if(!put(32*((int64_t)f.yields[i][0]*nutrient_value+(int64_t)f.yields[i][1]*mineral_value+((int64_t)f.yields[i][2]*f.efficiency+15)/16*energy_value)-i,score))return UpkeepWorkerAllocationError::overflow;
   if(score>best){best=score;choice=(int)i;}
  }
  if(choice<0)break;
  r.worked|=1u<<(unsigned)choice;r.added[r.added_count++]=(uint32_t)choice;
  if(!put((int64_t)r.nutrient_after+f.yields[choice][0],r.nutrient_after)||!put((int64_t)r.mineral_after+f.yields[choice][1],r.mineral_after)||!put((int64_t)r.energy_after+f.yields[choice][2],r.energy_after))return UpkeepWorkerAllocationError::overflow;
  --workers;
 }
 r.unallocated=workers;
 // Existing specialists also need technology/obsolescence and economy effects.
 r.requires_specialist_resolution=workers>0||r.specialists>0||r.added_count>0;
 r.requires_riot_resolution=f.can_riot;
 out=r;return UpkeepWorkerAllocationError::ok;
}
}
