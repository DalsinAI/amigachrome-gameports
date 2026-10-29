/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "human_worker_allocation.h"
#include <stdio.h>
#include <string.h>
static alpha::HumanAllocationResult& result_at(void* p){return *static_cast<alpha::HumanAllocationResult*>(p);}
static alpha::HumanAllocationTrace& trace_at(void* p){return *static_cast<alpha::HumanAllocationTrace*>(p);}
static bool emit(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i){b[i]=(uint8_t)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(){using namespace alpha;long long words[106];while(scanf("%lld",&words[0])==1){for(unsigned i=1;i<106;++i)if(scanf("%lld",&words[i])!=1)return 20;HumanAllocationFacts f{};unsigned j=0;f.variant=(Variant)words[j++];f.profile=(HumanAllocationProfile)words[j++];f.population=(int32_t)words[j++];f.requirement=(int32_t)words[j++];f.maintenance=(int32_t)words[j++];f.current_item=(int32_t)words[j++];f.specialists=(int32_t)words[j++];f.worked=(uint32_t)words[j++];f.unavailable=(uint32_t)words[j++];for(auto&x:f.satellites)x=(int32_t)words[j++];for(auto&x:f.specialist_types)x=(uint32_t)words[j++];f.minimum_specialist_size=(int32_t)words[j++];for(auto&c:f.citizens){c.available=(uint32_t)words[j++];c.economy=(int32_t)words[j++];c.psych=(int32_t)words[j++];c.labs=(int32_t)words[j++];}for(auto&y:f.yields)for(auto&x:y)x=(int32_t)words[j++];if(j!=106)return 20;
 HumanAllocationResult r,old;memset(&r,0x5a,sizeof r);memcpy(&old,&r,sizeof r);HumanAllocationTrace trace,trace_old;memset(&trace,0x6b,sizeof trace);memcpy(&trace_old,&trace,sizeof trace);auto error=human_worker_allocation_traced(f,r,trace);if(!emit((uint32_t)error))return 21;if(error!=WorkerAllocationError::ok){if(memcmp(&r,&old,sizeof r))return 21;for(unsigned i=0;i<10;++i)if(!emit(0x5a5a5a5a))return 21;}else{if(!emit(r.worked)||!emit(r.specialists)||!emit(r.adjustment))return 21;for(auto x:r.intake)if(!emit(x))return 21;for(auto x:r.specialist_types)if(!emit(x))return 21;if(!emit(r.exchanged)||!emit(r.food_deficit))return 21;}
 if(error!=WorkerAllocationError::ok){if(memcmp(&trace,&trace_old,sizeof trace)||!emit(0))return 21;}
 else {if(trace.count>allocation_visit_capacity||!emit(trace.count))return 21;for(unsigned i=0;i<trace.count;++i)if(!emit((uint32_t)trace.visits[i].kind)||!emit(trace.visits[i].tile))return 21;}
 HumanAllocationResult plain{};if(human_worker_allocation(f,plain)!=error)return 21;
 if(error==WorkerAllocationError::ok&&(plain.worked!=r.worked||plain.specialists!=r.specialists||plain.adjustment!=r.adjustment||plain.specialist_types[0]!=r.specialist_types[0]||plain.specialist_types[1]!=r.specialist_types[1]||plain.exchanged!=r.exchanged||plain.food_deficit!=r.food_deficit||memcmp(plain.intake,r.intake,sizeof r.intake)))return 21;
 auto facts_before=f;auto result_before=r;auto trace_before=trace;
 if(human_worker_allocation_traced(f,result_at(&f),trace)!=WorkerAllocationError::bad_buffer||memcmp(&f,&facts_before,sizeof f)||memcmp(&trace,&trace_before,sizeof trace))return 21;
 if(human_worker_allocation_traced(f,r,trace_at(&f))!=WorkerAllocationError::bad_buffer||memcmp(&f,&facts_before,sizeof f)||memcmp(&r,&result_before,sizeof r))return 21;
 if(human_worker_allocation_traced(f,result_at(&trace),trace)!=WorkerAllocationError::bad_buffer||memcmp(&trace,&trace_before,sizeof trace))return 21;
 // Alias rejection must precede every write and leave the facts unchanged.
 auto before=f;if(human_worker_allocation(f,result_at(&f))!=WorkerAllocationError::bad_buffer||memcmp(&f,&before,sizeof f))return 21;
 }return 0;}
