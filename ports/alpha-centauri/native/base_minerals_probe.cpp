/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_minerals.h"
#include <stdio.h>
#include <string.h>
static bool read32(int32_t&out){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;uint32_t n=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;memcpy(&out,&n,4);return true;}
static void write32(int32_t n){uint32_t v;memcpy(&v,&n,4);unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};fwrite(b,1,4,stdout);}
int main(){
 alpha::MineralFacts alias{};unsigned char before[sizeof alias];memcpy(before,&alias,sizeof alias);
 for(unsigned offset=0;offset+sizeof(alpha::MineralResult)<=sizeof alias;offset+=alignof(alpha::MineralResult)){
  auto*out=reinterpret_cast<alpha::MineralResult*>(reinterpret_cast<unsigned char*>(&alias)+offset);
  if(alpha::base_minerals(alias,*out)!=alpha::MineralError::bad_buffer||memcmp(before,&alias,sizeof alias))return 21;
 }
 int32_t v[31];while(read32(v[0])){
  for(unsigned i=1;i<31;++i)if(!read32(v[i]))return 20;
  for(unsigned i=18;i<31;++i)if(v[i]!=0&&v[i]!=1)return 20;
  alpha::MineralFacts f{v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14],v[15],v[16],v[17],
   v[18]!=0,v[19]!=0,v[20]!=0,v[21]!=0,v[22]!=0,v[23]!=0,v[24]!=0,v[25]!=0,v[26]!=0,v[27]!=0,v[28]!=0,v[29]!=0,v[30]!=0};
  alpha::MineralResult out;memset(&out,0x5a,sizeof out);auto error=alpha::base_minerals(f,out);
  write32((int32_t)error);write32(out.intake);write32(out.consumption);write32(out.surplus);write32(out.surplus_final);write32(out.eco_damage);
 }
 return ferror(stdin)||ferror(stdout)?20:0;
}
