/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "energy_inefficiency.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&n){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static bool write_word(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(n>>(8*i));return fwrite(b,1,4,stdout)==4;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;char magic[4];uint32_t count;if(fread(magic,1,4,f)!=4||memcmp(magic,"EIN1",4)||!read_word(f,count)||count>100000)return 20;
 for(uint32_t i=0;i<count;++i){uint32_t words[14];for(auto&v:words)if(!read_word(f,v))return 20;InefficiencyFacts facts{};facts.energy=(int32_t)words[0];facts.pending_efficiency=(int32_t)words[1];facts.hq_distance=(int32_t)words[2];facts.creche=words[3];facts.upkeep_stage=words[4];for(unsigned j=0;j<9;++j)facts.counters[j]=(int32_t)words[5+j];InefficiencyFacts original=facts;
  alignas(InefficiencyFacts) unsigned char storage[sizeof(InefficiencyFacts)+sizeof(InefficiencyResult)];memset(storage,0x4b,sizeof storage);memcpy(storage,&facts,sizeof facts);unsigned char old[sizeof storage];memcpy(old,storage,sizeof storage);auto&in=*reinterpret_cast<InefficiencyFacts*>(storage);
  const unsigned offsets[]={0,4,52};for(unsigned offset:offsets){auto&out=*reinterpret_cast<InefficiencyResult*>(storage+offset);if(energy_inefficiency(in,out)!=InefficiencyError::bad_buffer||memcmp(old,storage,sizeof storage))return 21;}
  InefficiencyResult result;memset(&result,0x5a,sizeof result);InefficiencyResult before;memcpy(&before,&result,sizeof result);const auto error=energy_inefficiency(facts,result);if(memcmp(&facts,&original,sizeof facts)||(error!=InefficiencyError::ok&&memcmp(&before,&result,sizeof result)))return 21;if(!write_word((uint32_t)error)||!write_word((uint32_t)result.loss))return 20;for(int32_t v:result.counters)if(!write_word((uint32_t)v))return 20;
 }const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;}
