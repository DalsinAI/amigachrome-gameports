/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_energy.h"
#include <stdio.h>
#include <string.h>
#include <initializer_list>
using namespace alpha;
static bool read_word(FILE*f,uint32_t&n){unsigned char b[4];if(fread(b,1,4,f)!=4)return false;n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);return true;}
static bool write_word(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(n>>(8*i));return fwrite(b,1,4,stdout)==4;}
static bool direct_checks(){
 BaseEnergyFacts f{};f.population.population=5;BaseEnergyResult out;memset(&out,0x5a,sizeof out);const auto before=out;
 f.population.police_count=1;if(base_energy(f,out)!=PsychError::bad_buffer||memcmp(&out,&before,sizeof out))return false;
 f.population.police_units=reinterpret_cast<const PoliceUnitFacts*>(&f);if(base_energy(f,out)!=PsychError::bad_buffer||memcmp(&out,&before,sizeof out))return false;
 f.population.police_count=0;f.population.police_units=nullptr;if(base_energy(f,out)!=PsychError::ok||out.population.drones!=2)return false;
 const auto valid=out;f.population.psych=INT32_MAX;if(base_energy(f,out)!=PsychError::ok||memcmp(&out,&valid,sizeof out))return false;
 static PoliceUnitFacts many[2048];f.population.police_units=many;f.population.police_count=2048;if(base_energy(f,out)!=PsychError::ok||out.population.drones!=1)return false;
 const auto accepted=out;many[2047].double_police=2;if(base_energy(f,out)!=PsychError::bad_state||memcmp(&out,&accepted,sizeof out))return false;many[2047].double_police=0;
 f.population.police_count=2049;if(base_energy(f,out)!=PsychError::bad_state||memcmp(&out,&accepted,sizeof out))return false;
 return true;
}
int main(int argc,char**argv){if(argc!=2)return 20;if(!direct_checks())return 21;FILE*f=fopen(argv[1],"rb");if(!f)return 20;char magic[4];uint32_t count;if(fread(magic,1,4,f)!=4||memcmp(magic,"BEE1",4)||!read_word(f,count)||count>100000)return 20;
 for(uint32_t i=0;i<count;++i){uint32_t w[229];for(auto&v:w)if(!read_word(f,v))return 20;BaseEnergyFacts input{};auto&facts=input.population;facts.expansion=w[0];facts.human=w[1];facts.upkeep_stage=w[2];facts.population=(int32_t)w[3];facts.specialists=(int32_t)w[4];facts.assimilation_turns=(int32_t)w[5];facts.difficulty=(int32_t)w[6];facts.pending_efficiency=(int32_t)w[7];facts.map_factor=(int32_t)w[8];facts.base_id=(int32_t)w[9];facts.faction_base_count=(int32_t)w[10];facts.rule_drone=(int32_t)w[11];facts.rule_talent=(int32_t)w[12];facts.has_nodrone=w[13];facts.nodrone=(int32_t)w[14];facts.pending_talent=(int32_t)w[15];facts.pending_police=(int32_t)w[16];facts.pacifism_count=(int32_t)w[17];facts.psych=(int32_t)w[18];facts.genejack_drones=(int32_t)w[19];facts.pending_economics=(int32_t)w[20];facts.nerve_staple_turns=(int32_t)w[21];facts.facilities[0]=w[22];facts.facilities[1]=w[23];facts.owned_projects=w[24];facts.worm_police=w[25];facts.police_count=w[26];PoliceUnitFacts units[8];for(unsigned j=0;j<82;++j)facts.social_psych[j]=(int32_t)w[27+j];for(unsigned j=0;j<8;++j){const unsigned o=109+j*5;units[j]={(int32_t)w[o],(int32_t)w[o+1],(int32_t)w[o+2],(int32_t)w[o+3],w[o+4]};}if(facts.police_count>8&&facts.police_count<=2048)return 20;facts.police_units=units;
  const uint32_t*a=w+149;auto&alloc=input.allocation;alloc.intake=(int32_t)a[0];alloc.convoy_to=(int32_t)a[1];alloc.convoy_from=(int32_t)a[2];for(unsigned j=0;j<7;++j)alloc.commerce[j]=(int32_t)a[3+j];alloc.allocation_psych=(int32_t)a[10];alloc.allocation_labs=(int32_t)a[11];alloc.pending_efficiency=(int32_t)a[12];alloc.hq_distance=(int32_t)a[13];alloc.creche=a[14];alloc.upkeep_stage=a[15];for(unsigned j=0;j<9;++j)alloc.counters[j]=(int32_t)a[16+j];alloc.faction_surplus=(int32_t)a[25];
  for(unsigned j=0;j<7;++j){const uint32_t*c=w+175+5*j;input.citizens[j]={c[0],c[1],(int32_t)c[2],(int32_t)c[3],(int32_t)c[4]};}
  const uint32_t*x=w+210;input.minimum_specialist_size=(int32_t)x[0];input.specialist_types[0]=x[1];input.specialist_types[1]=x[2];input.projects_here=x[3];input.rule_research=(int32_t)x[4];input.network_node_count=(int32_t)x[5];input.energy_grid_project_owned=x[6];input.alien=x[7];input.expansion_enabled=x[8];input.sanctioned=x[9];input.partner_aliens=x[10];input.partner_sanctions=x[11];input.qualified_foreign_commerce=x[12];input.facility_bits64_to69=x[13];input.projects_built_here=x[14];input.alien_grid_policy=(AlienGridPolicy)x[15];input.current_item=(int32_t)x[16];input.previous_item=(int32_t)x[17];input.mineral_surplus=(int32_t)x[18];
  const auto original=input;PoliceUnitFacts oldunits[8];memcpy(oldunits,units,sizeof units);
  alignas(BaseEnergyFacts) unsigned char storage[sizeof(BaseEnergyFacts)+sizeof(BaseEnergyResult)];memset(storage,0x4b,sizeof storage);memcpy(storage,&input,sizeof input);unsigned char old[sizeof storage];memcpy(old,storage,sizeof storage);auto&in=*reinterpret_cast<BaseEnergyFacts*>(storage);for(unsigned offset:{0u,4u,(unsigned)sizeof(BaseEnergyFacts)-4}){auto&out=*reinterpret_cast<BaseEnergyResult*>(storage+offset);if(base_energy(in,out)!=PsychError::bad_buffer||memcmp(old,storage,sizeof storage))return 21;}
  BaseEnergyResult result;memset(&result,0x5a,sizeof result);const auto before=result;const auto error=base_energy(input,result);if(memcmp(&input,&original,sizeof input)||memcmp(units,oldunits,sizeof units)||(error!=PsychError::ok&&memcmp(&before,&result,sizeof result)))return 21;
  if(facts.police_count>0&&facts.police_count<=8){alignas(BaseEnergyResult) unsigned char array_storage[sizeof units+sizeof(BaseEnergyResult)];memset(array_storage,0x31,sizeof array_storage);memcpy(array_storage,units,sizeof units);unsigned char old_array[sizeof array_storage];memcpy(old_array,array_storage,sizeof array_storage);auto alias=input;alias.population.police_units=reinterpret_cast<const PoliceUnitFacts*>(array_storage);auto&out=*reinterpret_cast<BaseEnergyResult*>(array_storage);if(base_energy(alias,out)!=PsychError::bad_buffer||memcmp(old_array,array_storage,sizeof array_storage))return 21;}
  if(!write_word((uint32_t)error))return 20;
  const auto&v=result.accounting;for(int32_t n:{v.intake,v.consumption,v.inefficiency,v.surplus,v.economy,v.psych,v.labs,v.unknown_total,v.faction_surplus})if(!write_word((uint32_t)n))return 20;for(int32_t n:v.counters)if(!write_word((uint32_t)n))return 20;
  for(const auto&row:result.population.rows)for(int32_t n:row)if(!write_word((uint32_t)n))return 20;
  for(int32_t n:{result.population.talents,result.population.drones,result.population.superdrones})if(!write_word((uint32_t)n))return 20;
  for(int32_t n:result.population.social_psych)if(!write_word((uint32_t)n))return 20;
  for(uint32_t n:result.specialist_types)if(!write_word(n))return 20;
 }const bool end=fgetc(f)==EOF;fclose(f);return end?0:20;}
