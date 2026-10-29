/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "upkeep_base_energy.h"
#include <stdio.h>
#include <string.h>
static bool read32(int32_t&out){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;const uint32_t n=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;memcpy(&out,&n,4);return true;}
static void write32(int32_t n){uint32_t v;memcpy(&v,&n,4);unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};fwrite(b,1,4,stdout);}
int main(){
 alpha::EnergyFacts alias{};unsigned char before[sizeof alias];memcpy(before,&alias,sizeof alias);
 for(unsigned offset=0;offset+sizeof(alpha::EnergyResult)<=sizeof alias;offset+=alignof(alpha::EnergyResult)){
  auto*out=reinterpret_cast<alpha::EnergyResult*>(reinterpret_cast<unsigned char*>(&alias)+offset);
  if(alpha::base_energy_accounting(alias,*out)!=alpha::EnergyError::bad_buffer||memcmp(before,&alias,sizeof alias))return 21;
 }
 int32_t v[18];while(read32(v[0])){
  for(unsigned i=1;i<18;++i)if(!read32(v[i]))return 20;
  if(v[8]!=0&&v[8]!=1)return 20;
  alpha::EnergyFacts f{v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8]!=0,{}};for(unsigned j=0;j<9;++j)f.social_efficiency_before[j]=v[9+j];
  alpha::EnergyResult r;memset(&r,0x5a,sizeof r);const auto e=alpha::base_energy_accounting(f,r);
  write32((int32_t)e);write32(r.intake);write32(r.consumption);write32(r.inefficiency);write32(r.surplus);for(int32_t n:r.social_efficiency_after)write32(n);
 }
 return ferror(stdin)||ferror(stdout)?20:0;
}
