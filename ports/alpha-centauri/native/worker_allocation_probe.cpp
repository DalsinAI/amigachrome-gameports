/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "worker_allocation.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
int main(){
 long x[75];for(;;){if(scanf("%ld",&x[0])!=1)return feof(stdin)?0:20;for(unsigned i=1;i<75;++i)if(scanf("%ld",&x[i])!=1)return 20;
  WorkerAllocationFacts f{};f.available=(uint32_t)x[0];f.worked=(uint32_t)x[1];int32_t*fields[]={&f.population,&f.specialists,&f.nutrient_before,&f.mineral_before,&f.energy_before,&f.efficiency,&f.psych_allocation,&f.nutrient_threshold};for(unsigned i=0;i<8;++i)*fields[i]=(int32_t)x[2+i];f.can_grow=x[10]!=0;f.can_riot=x[11]!=0;for(unsigned i=0;i<21;++i)for(unsigned k=0;k<3;++k)f.yields[i][k]=(int32_t)x[12+3*i+k];
  WorkerAllocationResult r;memset(&r,0x5a,sizeof r);WorkerAllocationResult before;memcpy(&before,&r,sizeof r);WorkerAllocationFacts old=f;
  const auto error=worker_allocation_candidate(f,r);if(memcmp(&f,&old,sizeof f)||(error!=WorkerAllocationError::ok&&memcmp(&r,&before,sizeof r)))return 21;
  void*alias=&f;
  if(worker_allocation_candidate(f,*static_cast<WorkerAllocationResult*>(alias))!=WorkerAllocationError::bad_buffer||memcmp(&f,&old,sizeof f))return 21;
  printf("%d",(int)error);if(error==WorkerAllocationError::ok){printf(" %lu %ld %ld %ld %ld %ld %lu %d %d %d",(unsigned long)r.worked,(long)r.specialists,(long)r.unallocated,(long)r.nutrient_after,(long)r.mineral_after,(long)r.energy_after,(unsigned long)r.added_count,r.reset_overassigned,r.requires_specialist_resolution,r.requires_riot_resolution);for(uint32_t i:r.added)printf(" %lu",(unsigned long)i);}puts("");
 }
}
