/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "original_human_cost.h"
#include <stdio.h>
#include <string.h>
static int32_t load(const unsigned char* b){const uint32_t n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char b[24];size_t count=0;
 for(;;){const size_t n=fread(b,1,sizeof b,f);if(!n){const bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>100000){fclose(f);return 20;}
  const int32_t flags=load(b+20);alpha::OriginalHumanCosts output{0x31415926,0x31415926};
  alpha::LogicError error=alpha::LogicError::bad_state;
  if(flags>=0 && flags<=3){const alpha::OriginalHumanCostInput in{load(b),load(b+4),load(b+8),load(b+12),load(b+16),(flags&1)!=0,(flags&2)!=0};error=alpha::original_human_costs(in,output);}
  if(error!=alpha::LogicError::ok && (output.nutrient!=0x31415926 || output.mineral!=0x31415926)){fclose(f);return 21;}
  if(!emit((int32_t)error) || !emit(output.nutrient) || !emit(output.mineral)){fclose(f);return 20;}
 }
}
