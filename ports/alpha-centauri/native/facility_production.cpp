/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in facility_production.h. */
#include "facility_production.h"
#include <stddef.h>
#include <limits.h>
namespace alpha { namespace {
bool overlap(const void* a,size_t n,const void* b,size_t m){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n>UINTPTR_MAX-x || m>UINTPTR_MAX-y || (x<y+m && y<x+n);
}
bool fits(int64_t n){return n>=INT32_MIN && n<=INT32_MAX;}
}
FacilityProductionError facility_production(const FacilityProductionFacts& in,FacilityProductionResult& output){
 if(overlap(&in,sizeof(in),&output,sizeof(output)))return FacilityProductionError::bad_buffer;
 if(in.accumulated<0 || in.baseline<0 || in.mineral_intake<0 || in.cost_units<0 || in.cost_factor<1 ||
    in.exemption<0 || in.exemption>1000 || in.best_output<0)return FacilityProductionError::bad_state;
 const int64_t twice=(int64_t)in.surplus*2,total_output=(int64_t)in.mineral_intake+twice;
 if(!fits(twice) || !fits(total_output))return FacilityProductionError::overflow;
 FacilityProductionResult next{in.accumulated,in.baseline,0,0,in.best_output,in.state_flags&~0x00800000u,false};
 if(total_output>next.best_output)next.best_output=(int32_t)total_output;
 if(in.starved){output=next;return FacilityProductionError::ok;}
 if(in.surplus>=0 && !in.riots){
  const int64_t total=(int64_t)next.accumulated+in.surplus;
  if(!fits(total))return FacilityProductionError::overflow;
  next.accumulated=(int32_t)total;next.credited=in.surplus;
 }
 const int64_t cost=(int64_t)in.cost_units*in.cost_factor;
 if(!fits(cost))return FacilityProductionError::overflow;
 next.cost=(int32_t)cost;
 if(next.accumulated>=next.cost){
  next.accumulated-=next.cost;
  const int32_t cap=in.exemption<in.surplus?in.exemption:in.surplus;
  if(in.human && next.accumulated>cap)next.accumulated=cap>0?cap:0;
  next.baseline=next.accumulated;next.completed=true;
  next.state_flags=(next.state_flags|0x00800000u)&~0x80000000u;
 }
 output=next;return FacilityProductionError::ok;
}
const char* facility_production_error_name(FacilityProductionError e){switch(e){
 case FacilityProductionError::ok:return "ok";case FacilityProductionError::bad_state:return "bad_state";
 case FacilityProductionError::bad_buffer:return "bad_buffer";case FacilityProductionError::overflow:return "overflow";
 case FacilityProductionError::not_human:return "not_human";case FacilityProductionError::not_owner:return "not_owner";
 case FacilityProductionError::unsupported:return "unsupported";case FacilityProductionError::unavailable:return "unavailable";
 case FacilityProductionError::already_processed:return "already_processed";case FacilityProductionError::stale_world:return "stale_world";
 }return "unknown";}
}
