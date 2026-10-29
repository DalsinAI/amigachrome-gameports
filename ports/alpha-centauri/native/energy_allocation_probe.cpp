/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_allocation.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&n){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static bool write_word(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(n>>(8*i));return fwrite(b,1,4,stdout)==4;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;char magic[4];uint32_t count;if(fread(magic,1,4,f)!=4||memcmp(magic,"ENA1",4)||!read_word(f,count)||count>100000)return 20;
 for(uint32_t i=0;i<count;++i){uint32_t w[26];for(auto&v:w)if(!read_word(f,v))return 20;EnergyAllocationFacts facts{};facts.intake=(int32_t)w[0];facts.convoy_to=(int32_t)w[1];facts.convoy_from=(int32_t)w[2];for(unsigned j=0;j<7;++j)facts.commerce[j]=(int32_t)w[3+j];facts.allocation_psych=(int32_t)w[10];facts.allocation_labs=(int32_t)w[11];facts.pending_efficiency=(int32_t)w[12];facts.hq_distance=(int32_t)w[13];facts.creche=w[14];facts.upkeep_stage=w[15];for(unsigned j=0;j<9;++j)facts.counters[j]=(int32_t)w[16+j];facts.faction_surplus=(int32_t)w[25];const auto original=facts;
  alignas(EnergyAllocationFacts) unsigned char storage[sizeof(EnergyAllocationFacts)+sizeof(EnergyAllocationResult)];memset(storage,0x4b,sizeof storage);memcpy(storage,&facts,sizeof facts);unsigned char old[sizeof storage];memcpy(old,storage,sizeof storage);auto&in=*reinterpret_cast<EnergyAllocationFacts*>(storage);const unsigned offsets[]={0,4,100};for(unsigned offset:offsets){auto&out=*reinterpret_cast<EnergyAllocationResult*>(storage+offset);if(energy_allocation(in,out)!=InefficiencyError::bad_buffer||memcmp(old,storage,sizeof storage))return 21;}
  EnergyAllocationResult result;memset(&result,0x5a,sizeof result);const auto before=result;const auto error=energy_allocation(facts,result);if(memcmp(&facts,&original,sizeof facts)||(error!=InefficiencyError::ok&&memcmp(&before,&result,sizeof result)))return 21;
  const int32_t values[]={result.intake,result.consumption,result.inefficiency,result.surplus,result.economy,result.psych,result.labs,result.unknown_total,result.faction_surplus};if(!write_word((uint32_t)error))return 20;for(int32_t v:values)if(!write_word((uint32_t)v))return 20;for(int32_t v:result.counters)if(!write_word((uint32_t)v))return 20;
 }const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;}
