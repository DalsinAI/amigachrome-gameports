/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_allocation.h"
#include <stdio.h>
#include <initializer_list>
#include <string.h>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&out){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;out=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static void word(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;uint32_t count;if(!read_word(f,count)||count>100000)return 20;
 for(uint32_t k=0;k<count;++k){uint32_t w[76];for(auto&n:w)if(!read_word(f,n))return 20;unsigned p=0;AllocationFacts a{};
  a.fresh_surplus=(int32_t)w[p++];a.alloc_labs=(int32_t)w[p++];a.alloc_psych=(int32_t)w[p++];a.pending_efficiency=(int32_t)w[p++];a.population=(int32_t)w[p++];a.specialist_count=(int32_t)w[p++];a.min_specialist_population=(int32_t)w[p++];for(auto&n:a.specialist_types)n=w[p++];
  for(auto&n:a.prerequisite_known)n=(uint8_t)w[p++];
  for(auto&n:a.obsolete_known)n=(uint8_t)w[p++];
  a.citizens.count=w[p++];for(auto&c:a.citizens.specialists){c.prerequisite=(int32_t)w[p++];c.obsolete=(int32_t)w[p++];c.economy=(int32_t)w[p++];c.psych=(int32_t)w[p++];c.labs=(int32_t)w[p++];}
  bool*flags[]={&a.biology_lab,&a.energy_bank,&a.network_node,&a.hologram_theatre,&a.research_hospital,&a.nanohospital,&a.tree_farm,&a.hybrid_forest,&a.fusion_lab,&a.quantum_lab,&a.supercollider,&a.theory_everything,&a.space_elevator,&a.longevity_free_market,&a.network_backbone};for(auto*b:flags)*b=w[p++]!=0;
  a.global_network_nodes=(int32_t)w[p++];a.commerce=(int32_t)w[p++];if(p!=76)return 21;
  const auto original=a;if(allocate_energy(a,*reinterpret_cast<AllocationResult*>(&a))!=EnergyError::bad_buffer||memcmp(&a,&original,sizeof a))return 21;
  AllocationResult r,unchanged;memset(&r,0x5a,sizeof r);memcpy(&unchanged,&r,sizeof r);const auto error=allocate_energy(a,r);if(error!=EnergyError::ok&&memcmp(&r,&unchanged,sizeof r))return 21;if(memcmp(&a,&original,sizeof a))return 21;
  word((uint32_t)error);for(int32_t n:{r.distributed_energy,r.economy,r.psych,r.labs,r.unknown_total,r.coefficient_economy,r.coefficient_psych,r.coefficient_labs})word((uint32_t)n);for(auto n:r.specialist_types)word(n);for(int32_t n:{r.specialist_economy,r.specialist_psych,r.specialist_labs})word((uint32_t)n);
 }
 const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;
}
