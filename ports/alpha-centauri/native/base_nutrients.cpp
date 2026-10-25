/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in header. */
#include "base_nutrients.h"
namespace alpha {namespace {
bool checked(int64_t n,int32_t&out){if(n<(-2147483647LL-1)||n>2147483647LL)return false;out=(int32_t)n;return true;}
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;return x<=y?y-x<an:x-y<bn;}
}
NutrientError base_nutrients(const NutrientFacts&f,NutrientResult&output){
 if(overlap(&f,sizeof f,&output,sizeof output))return NutrientError::bad_buffer;
 if(f.population<0||f.population>127||f.requirement<0||(f.flags&~3u))return NutrientError::bad_state;
 NutrientResult next{};next.growth=f.pending_growth;
 if((f.flags&1u)&&!checked((int64_t)next.growth+2,next.growth))return NutrientError::overflow;
 if((f.flags&2u)&&!checked((int64_t)next.growth+2,next.growth))return NutrientError::overflow;
 if(!checked((int64_t)f.intake+f.convoy_to,next.intake)||!checked((int64_t)f.population*f.requirement,next.consumption)||!checked((int64_t)next.consumption+f.convoy_from,next.consumption)||!checked((int64_t)next.intake-next.consumption,next.surplus))return NutrientError::overflow;
 next.stored=f.stored;if(next.surplus>=0&&next.stored<0)next.stored=0;else if(next.surplus<0&&next.stored==0)next.stored=-1;
 next.faction_surplus=f.faction_surplus;
 if(f.upkeep_stage==1){const int32_t contribution=next.surplus<0?0:next.surplus>99?99:next.surplus;if(!checked((int64_t)next.faction_surplus+contribution,next.faction_surplus))return NutrientError::overflow;}
 output=next;return NutrientError::ok;
}
}
