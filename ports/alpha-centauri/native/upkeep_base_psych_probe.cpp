/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "upkeep_base_psych.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
using namespace alpha;
static bool word(uint32_t&v){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return true;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i)b[i]=(unsigned char)(v>>(8*i));return fwrite(b,1,4,stdout)==4;}
static PsychResult&alias(void*p){return *static_cast<PsychResult*>(p);}
int main(){uint32_t count;if(!word(count)||count>1000000)return 20;for(uint32_t n=0;n<count;++n){
 uint32_t v[21];for(auto&x:v)if(!word(x))return 20;if(v[20]&~8191u)return 20;
 PsychFacts f{};int32_t*values[]={&f.population,&f.specialists,&f.content_population,&f.base_limit,&f.base_index,&f.faction_base_count,&f.difficulty,&f.assimilation_turns,&f.rule_drone,&f.rule_talent,&f.no_drone_bonus,&f.pending_talent,&f.pending_police,&f.fresh_psych,&f.genejack_drones,&f.pacifism_units,&f.police_effects[0],&f.police_effects[1],&f.police_effects[2],&f.pending_economics};for(unsigned i=0;i<20;++i)*values[i]=(int32_t)v[i];
 bool*flags[]={&f.genejack,&f.recreation_commons,&f.hologram_or_virtual,&f.research_hospital,&f.nanohospital,&f.paradise_garden,&f.planetary_transit,&f.nerve_stapled,&f.punishment_sphere,&f.telepathic_matrix,&f.longevity_vaccine,&f.human_genome,&f.clinical_immortality};for(unsigned i=0;i<13;++i)*flags[i]=(v[20]&(1u<<i))!=0;
 PsychFacts original;memcpy(&original,&f,sizeof f);PsychResult out,old;memset(&out,0x5a,sizeof out);memcpy(&old,&out,sizeof out);const auto e=base_psych_policy(f,out);
 if(memcmp(&f,&original,sizeof f)||(e!=UpkeepPsychError::ok&&memcmp(&out,&old,sizeof out)))return 21;
 if(base_psych_policy(f,alias(&f))!=UpkeepPsychError::bad_buffer||memcmp(&f,&original,sizeof f))return 21;
 if(!emit((uint32_t)e))return 20;
 if(e!=UpkeepPsychError::ok){for(unsigned i=0;i<23;++i)if(!emit(0x5a5a5a5au))return 20;continue;}
 const int32_t summary[]={out.talents,out.drones,out.superdrones,out.efficiency_drones,out.capture_drones,out.police_total};for(auto x:summary)if(!emit((uint32_t)x))return 20;for(auto&row:out.rows)for(auto x:row)if(!emit((uint32_t)x))return 20;if(!emit(out.can_riot)||!emit(out.will_riot))return 20;
 }if(fgetc(stdin)!=EOF)return 20;return 0;}
