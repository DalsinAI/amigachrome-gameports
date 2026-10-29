/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in facility_upkeep.h. */
#include "facility_upkeep.h"
#include <string.h>
namespace alpha {namespace {
bool put(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool overlap(const void*a,unsigned an,const void*b,unsigned bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
bool bit(const uint8_t*b,int id){return(b[(unsigned)id/8]&(1u<<(id%8)))!=0;}
}
EnergyError facility_upkeep(const FacilityUpkeepFacts&f,FacilityUpkeepResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return EnergyError::bad_buffer;
 if((f.first_project!=39&&f.first_project!=70)||f.difficulty<0||f.difficulty>5||f.best_reactor<1||f.best_reactor>4||f.credits_before<0||f.maintenance_before<0||f.mineral_cost_factor<0)return EnergyError::bad_state;
 FacilityUpkeepResult r{};r.credits_after=f.credits_before;r.maintenance_after=f.maintenance_before;r.player_flags_after=f.player_flags_before;memcpy(r.built_after,f.built,sizeof f.built);
 const int repeatable=f.first_project==39?34:65;
 for(int id=1;id<repeatable;++id)if(bit(f.built,id)){
  if(f.cost[id]<0||f.maintenance[id]<0)return EnergyError::bad_state;
  int32_t maint=id==27?(f.best_reactor<(f.difficulty+1)/2?f.best_reactor:(f.difficulty+1)/2):bit(f.free_maintenance,id)?0:f.maintenance[id];
  if(f.self_aware_colony){if((r.player_flags_after&0x20u)&&!put((int64_t)maint+1,maint))return EnergyError::overflow;
   if(maint&1)r.player_flags_after|=0x20u;else r.player_flags_after&=~0x20u;maint/=2;
  }
  r.charged[id]=maint;
  if(!put((int64_t)r.credits_after-maint,r.credits_after)||!put((int64_t)r.maintenance_after+maint,r.maintenance_after))return EnergyError::overflow;
  if(r.credits_after<0){r.shortage[id/8]|=(uint8_t)(1u<<(id%8));
   if(f.difficulty<=1||f.queue_item==-id)r.credits_after=0;
   else{r.built_after[id/8]&=(uint8_t)~(1u<<(id%8));r.scrapped[id/8]|=(uint8_t)(1u<<(id%8));if(!put((int64_t)f.cost[id]*f.mineral_cost_factor,r.credits_after))return EnergyError::overflow;}
  }
 }
 output=r;return EnergyError::ok;
}
}
