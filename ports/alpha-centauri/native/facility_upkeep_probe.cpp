/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_upkeep.h"
#include <stdio.h>
#include <string.h>
static bool read32(int32_t&out){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;uint32_t n=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;memcpy(&out,&n,4);return true;}
static void write32(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static void bytes(uint8_t*out,const int32_t*in){for(unsigned j=0;j<3;++j){uint32_t n=(uint32_t)in[j];for(unsigned k=0;k<4;++k)out[j*4+k]=(uint8_t)(n>>(8*k));}}
static void words(const uint8_t*in){for(unsigned j=0;j<3;++j){const auto*b=in+4*j;write32((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);}}
int main(){alpha::FacilityUpkeepFacts alias{};unsigned char before[sizeof alias];memcpy(before,&alias,sizeof alias);
 for(unsigned offset=0;offset+sizeof(alpha::FacilityUpkeepResult)<=sizeof alias;offset+=alignof(alpha::FacilityUpkeepResult)){auto*out=reinterpret_cast<alpha::FacilityUpkeepResult*>(reinterpret_cast<unsigned char*>(&alias)+offset);if(alpha::facility_upkeep(alias,*out)!=alpha::EnergyError::bad_buffer||memcmp(before,&alias,sizeof alias))return 21;}
 int32_t v[155];while(read32(v[0])){for(unsigned i=1;i<155;++i)if(!read32(v[i]))return 20;if(v[8]!=0&&v[8]!=1)return 20;
  alpha::FacilityUpkeepFacts f{};f.first_project=v[0];f.difficulty=v[1];f.best_reactor=v[2];f.credits_before=v[3];f.maintenance_before=v[4];f.mineral_cost_factor=v[5];f.queue_item=v[6];f.player_flags_before=(uint32_t)v[7];f.self_aware_colony=v[8]!=0;bytes(f.built,v+9);bytes(f.free_maintenance,v+12);for(unsigned i=0;i<70;++i){f.cost[i]=v[15+i];f.maintenance[i]=v[85+i];}
  alpha::FacilityUpkeepResult r;memset(&r,0x5a,sizeof r);const auto e=alpha::facility_upkeep(f,r);write32((uint32_t)e);write32((uint32_t)r.credits_after);write32((uint32_t)r.maintenance_after);write32(r.player_flags_after);words(r.built_after);words(r.scrapped);words(r.shortage);for(int32_t x:r.charged)write32((uint32_t)x);
 }return ferror(stdin)||ferror(stdout)?20:0;}
