/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker default-policy adaptation; attribution and scope in base_support.h.
 */
#include "base_support.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
constexpr uint32_t support_bit=0x10u,drone=0x200000u,skip=0x400000u;
uint32_t account(const SupportUnit&u,const SupportFacts&f,SupportResult&r){
 if(!u.homed_here)return u.state;
 uint32_t state=u.state&~(support_bit|drone|skip);
 if(u.plan>9)return state;
 if(!u.clean_reactor&&!u.native_on_fungus){
  ++r.supported;
  if(r.supported>r.free_allowance){++r.paid_units;r.mineral_cost+=r.cost_per_paid_unit;state|=support_bit;}
 }
 if(u.offense!=0&&(u.outside_own_territory_and_base||(u.triad==2&&u.plan!=4))){
  ++r.pacifism_units;
  if(f.police_pending==-3&&r.pacifism_units==1){++r.free_skips;state|=skip;}
  else if(f.police_pending<=-3){++r.drone_units;state|=drone;}
 }
 return state;
}
}
SupportError base_support_policy(const SupportFacts&f,const SupportUnit*u,size_t count,
 SupportResult&output,uint32_t*states,size_t capacity){
 if(count>SIZE_MAX/sizeof(SupportUnit)||count>SIZE_MAX/sizeof(uint32_t))return SupportError::bad_state;
 const size_t bytes=count*sizeof(uint32_t),source=count*sizeof(SupportUnit);
 if((count&&(!u||!states))||capacity<count)return SupportError::bad_buffer;
 if(overlap(&output,sizeof output,&f,sizeof f)||overlap(&output,sizeof output,u,source)||
 overlap(states,bytes,&f,sizeof f)||overlap(states,bytes,u,source)||overlap(states,bytes,&output,sizeof output))return SupportError::bad_buffer;
 if(f.population<0||f.population>127)return SupportError::bad_state;
 if(count>1073741823u)return SupportError::overflow;
 for(size_t i=0;i<count;++i)if(u[i].plan<0||u[i].plan>14||u[i].triad<0||u[i].triad>2)return SupportError::bad_state;
 const int index=f.support_pending<=-4?0:f.support_pending>=3?7:f.support_pending+4;
 static const int free_count[]={0,0,1,1,2,3,4,-4};
 SupportResult next{};next.free_allowance=free_count[index];
 if(next.free_allowance<0)next.free_allowance=f.population>4?f.population:4;
 next.cost_per_paid_unit=index==0?2:1;
 // The count bound above guarantees all eight signed totals fit, even at -4.
 for(size_t i=0;i<count;++i)states[i]=account(u[i],f,next);
 output=next;return SupportError::ok;
}
}
