/* SPDX-License-Identifier: GPL-3.0-or-later. Independently reconstructed
 * original SMAC/SMAX human base_yield selection, privately checked originals.
 */
#include "human_worker_allocation.h"
#include <limits.h>
namespace alpha {namespace {
struct Arithmetic {
 bool ok=true;
 int32_t value(int64_t v){if(v<INT32_MIN||v>INT32_MAX){ok=false;return 0;}return (int32_t)v;}
 int32_t add(int32_t a,int32_t b){return value((int64_t)a+b);}
 int32_t sub(int32_t a,int32_t b){return value((int64_t)a-b);}
 int32_t mul(int32_t a,int32_t b){return value((int64_t)a*b);}
};
int32_t clamp(int32_t n){return n<1?1:n>99?99:n;}
unsigned bits(uint32_t n){unsigned out=0;while(n){n&=n-1;++out;}return out;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<y+bn&&y<x+an;}
int32_t score(const HumanAllocationFacts& f,const int32_t totals[3],int32_t remaining,unsigned tile,Arithmetic& a){
 const int32_t need=a.mul(f.population,f.requirement);int32_t n=f.yields[tile][0],m=f.yields[tile][1],e=f.yields[tile][2],bonus=0;
 if(a.add(remaining/f.requirement,totals[0])<need){
  bonus=a.add(totals[0],remaining)>=need?2:8;
  if(n>=0){if(n<=bonus)n=a.mul(n,2);else n=a.add(n,bonus);}
 }
 n=a.mul(n,2);m=a.mul(m,2);e=a.mul(e,2);
 if(totals[0]<=a.add(f.population,need)&&n==0){m/=2;e/=2;}
 if(m==0&&bonus==0)n/=2;
 const int32_t food=clamp(a.sub(a.mul(a.add(a.sub(a.sub(totals[0],f.population/3),need),remaining),2),2));
 const int32_t mineral=clamp(a.add(a.sub(totals[1],f.maintenance),1)/2),energy=clamp(totals[2]);
 const int32_t coefficient=f.current_item<-(f.variant==Variant::base?39:70)?8:6;
 const int32_t ns=a.mul(a.mul(n,4),256)/food;
 const int32_t ms=a.mul(a.mul(a.mul(coefficient,f.population),m),64)/mineral;
 const int32_t es=a.mul(a.mul(f.population,e),64)/energy;
 return a.add(a.add(ns,ms),es);
}
}
WorkerAllocationError human_worker_allocation(const HumanAllocationFacts& f,HumanAllocationResult& output){
 if(overlap(&f,sizeof f,&output,sizeof output))return WorkerAllocationError::bad_buffer;
 if(f.profile!=HumanAllocationProfile::directed_no_governor_priorities)return WorkerAllocationError::unsupported;
 if((f.variant!=Variant::base&&f.variant!=Variant::alien_crossfire)||f.population<0||f.population>127||f.requirement<1||f.requirement>99||
    f.specialists<0||f.specialists>f.population||(f.worked&~0x1fffffu)||(f.unavailable&~0x1ffffffu)||
    bits(f.worked)+(unsigned)f.specialists>(unsigned)f.population+1||f.minimum_specialist_size<0)return WorkerAllocationError::bad_state;
 for(auto x:f.satellites)if(x<0||x>f.population)return WorkerAllocationError::bad_state;
 for(const auto&c:f.citizens)if(c.available>1)return WorkerAllocationError::bad_state;
 Arithmetic a;HumanAllocationResult next{};for(unsigned k=0;k<3;++k)next.intake[k]=f.satellites[k];
 next.specialist_types[0]=f.specialist_types[0];next.specialist_types[1]=f.specialist_types[1];
 const uint32_t initial=f.worked&(~f.unavailable|1u);int32_t remaining=f.population+1;
 auto apply=[&](unsigned at,bool remove){for(unsigned k=0;k<3;++k)next.intake[k]=remove?a.sub(next.intake[k],f.yields[at][k]):a.add(next.intake[k],f.yields[at][k]);};
 for(unsigned i=0;i<21;++i)if(!i||(initial&(1u<<i))){next.worked|=1u<<i;apply(i,false);if(--remaining==0)break;}
 if(remaining!=f.specialists){
  do {
   const bool complete=remaining<=f.specialists;int32_t best=0,worst=9999;unsigned best_id=0;int32_t worst_id=-1;
   const uint32_t excluded=complete?0:next.worked;
   for(unsigned i=1;i<21;++i)if(!((excluded|f.unavailable)&(1u<<i))){
    const int32_t value=score(f,next.intake,remaining,i,a);
    if(!(next.worked&(1u<<i))){if(value>best){best=value;best_id=i;}}
    else if(value<worst){worst=value;worst_id=(int32_t)i;}
   }
   if(!a.ok)return WorkerAllocationError::overflow;
   if(complete){
    if(best_id&&worst_id>0&&worst<best){
     const int32_t before[3]={next.intake[0],next.intake[1],next.intake[2]};
     apply((unsigned)worst_id,true);apply(best_id,false);
     if(next.intake[0]<a.mul(f.population+1,f.requirement)){for(unsigned k=0;k<3;++k)next.intake[k]=before[k];}
     else{next.worked=(next.worked&~(1u<<(unsigned)worst_id))|(1u<<best_id);next.exchanged=true;}
    }break;
   }
   if(!best_id)break;
   next.worked|=1u<<best_id;apply(best_id,false);--remaining;
  }while(f.specialists<remaining||next.intake[0]<a.mul(f.population,f.requirement));
 }
 if(remaining>f.specialists){
  int32_t best=-999;unsigned id=0;
  for(unsigned i=0;i<7;++i)if(f.citizens[i].available){
   int32_t value=a.mul(f.citizens[i].psych,3);
   if(f.population>=f.minimum_specialist_size)value=a.add(a.add(value,f.citizens[i].economy),f.citizens[i].labs);
   if(value>best){best=value;id=i;}
  }
  for(int32_t i=f.specialists;i<remaining&&i<16;++i){const unsigned shift=((unsigned)i&7)*4,word=(unsigned)i>>3;next.specialist_types[word]=(next.specialist_types[word]&~(15u<<shift))|(id<<shift);}
 }
 next.specialists=remaining;next.adjustment=0;next.food_deficit=next.intake[0]<a.mul(f.population,f.requirement);
 if(!a.ok)return WorkerAllocationError::overflow;
 output=next;return WorkerAllocationError::ok;
}
}
