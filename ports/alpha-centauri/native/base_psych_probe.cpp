/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_psych.h"
#include <stdio.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&n){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static bool write_word(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(n>>(8*i));return fwrite(b,1,4,stdout)==4;}
static bool direct_checks(){
 BasePsychFacts f{};f.population=5;BasePsychResult out;memset(&out,0x5a,sizeof out);const auto before=out;
 f.police_count=1;if(base_psych(f,out)!=PsychError::bad_buffer||memcmp(&out,&before,sizeof out))return false;
 f.police_units=reinterpret_cast<const PoliceUnitFacts*>(&f);if(base_psych(f,out)!=PsychError::bad_buffer||memcmp(&out,&before,sizeof out))return false;
 f.police_count=0;f.police_units=nullptr;if(base_psych(f,out)!=PsychError::ok||out.drones!=2)return false;
 static PoliceUnitFacts many[2048];f.police_units=many;f.police_count=2048;if(base_psych(f,out)!=PsychError::ok||out.drones!=1)return false;
 const auto accepted=out;many[2047].double_police=2;if(base_psych(f,out)!=PsychError::bad_state||memcmp(&out,&accepted,sizeof out))return false;many[2047].double_police=0;
 f.police_count=2049;if(base_psych(f,out)!=PsychError::bad_state||memcmp(&out,&accepted,sizeof out))return false;
 return true;
}
int main(int argc,char**argv){if(argc!=2)return 20;if(!direct_checks())return 21;FILE*f=fopen(argv[1],"rb");if(!f)return 20;char magic[4];uint32_t count;if(fread(magic,1,4,f)!=4||memcmp(magic,"PSY1",4)||!read_word(f,count)||count>100000)return 20;
 for(uint32_t i=0;i<count;++i){uint32_t w[149];for(auto&v:w)if(!read_word(f,v))return 20;BasePsychFacts facts{};facts.expansion=w[0];facts.human=w[1];facts.upkeep_stage=w[2];facts.population=(int32_t)w[3];facts.specialists=(int32_t)w[4];facts.assimilation_turns=(int32_t)w[5];facts.difficulty=(int32_t)w[6];facts.pending_efficiency=(int32_t)w[7];facts.map_factor=(int32_t)w[8];facts.base_id=(int32_t)w[9];facts.faction_base_count=(int32_t)w[10];facts.rule_drone=(int32_t)w[11];facts.rule_talent=(int32_t)w[12];facts.has_nodrone=w[13];facts.nodrone=(int32_t)w[14];facts.pending_talent=(int32_t)w[15];facts.pending_police=(int32_t)w[16];facts.pacifism_count=(int32_t)w[17];facts.psych=(int32_t)w[18];facts.genejack_drones=(int32_t)w[19];facts.pending_economics=(int32_t)w[20];facts.nerve_staple_turns=(int32_t)w[21];facts.facilities[0]=w[22];facts.facilities[1]=w[23];facts.owned_projects=w[24];facts.worm_police=w[25];facts.police_count=w[26];PoliceUnitFacts units[8];for(unsigned j=0;j<82;++j)facts.social_psych[j]=(int32_t)w[27+j];for(unsigned j=0;j<8;++j){const unsigned o=109+j*5;units[j]={(int32_t)w[o],(int32_t)w[o+1],(int32_t)w[o+2],(int32_t)w[o+3],w[o+4]};}if(facts.police_count>8&&facts.police_count<=2048)return 20;facts.police_units=units;const auto original=facts;PoliceUnitFacts oldunits[8];memcpy(oldunits,units,sizeof units);
  alignas(BasePsychFacts) unsigned char storage[sizeof(BasePsychFacts)+sizeof(BasePsychResult)];memset(storage,0x4b,sizeof storage);memcpy(storage,&facts,sizeof facts);unsigned char old[sizeof storage];memcpy(old,storage,sizeof storage);auto&in=*reinterpret_cast<BasePsychFacts*>(storage);const unsigned offsets[]={0,4,(unsigned)sizeof(BasePsychFacts)-4};for(unsigned offset:offsets){auto&out=*reinterpret_cast<BasePsychResult*>(storage+offset);if(base_psych(in,out)!=PsychError::bad_buffer||memcmp(old,storage,sizeof storage))return 21;}
  BasePsychResult result;memset(&result,0x5a,sizeof result);const auto before=result;const auto error=base_psych(facts,result);if(memcmp(&facts,&original,sizeof facts)||memcmp(units,oldunits,sizeof units)||(error!=PsychError::ok&&memcmp(&before,&result,sizeof result)))return 21;
  // Check overlap with the caller's garrison array, independently of facts.
  if(facts.police_count>0&&facts.police_count<=8){alignas(BasePsychResult) unsigned char array_storage[sizeof units+sizeof(BasePsychResult)];memset(array_storage,0x31,sizeof array_storage);memcpy(array_storage,units,sizeof units);unsigned char old_array[sizeof array_storage];memcpy(old_array,array_storage,sizeof array_storage);auto alias=facts;alias.police_units=reinterpret_cast<const PoliceUnitFacts*>(array_storage);auto&out=*reinterpret_cast<BasePsychResult*>(array_storage);if(base_psych(alias,out)!=PsychError::bad_buffer||memcmp(old_array,array_storage,sizeof array_storage))return 21;}
  if(!write_word((uint32_t)error))return 20;
  for(const auto&row:result.rows)for(int32_t v:row)if(!write_word((uint32_t)v))return 20;
  for(int32_t v:{result.talents,result.drones,result.superdrones})if(!write_word((uint32_t)v))return 20;
  for(int32_t v:result.social_psych)if(!write_word((uint32_t)v))return 20;
 }const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;}
