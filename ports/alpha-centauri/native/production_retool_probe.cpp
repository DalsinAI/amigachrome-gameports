/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "production_retool.h"
#include <stdio.h>
#include <string.h>
static int32_t load(const unsigned char* b){const uint32_t n=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){
 if(argc!=2)return 20;
 FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char b[44];size_t count=0;
 for(;;){const size_t n=fread(b,1,sizeof b,f);if(!n){const bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>200000){fclose(f);return 20;}
  const int32_t profile=load(b),human=load(b+16);alpha::ProductionRetoolResult output{0x31415926,0x31415926,0x31415926};
  alpha::LogicError error=alpha::LogicError::bad_state;
  if((profile==0 || profile==1) && (human==0 || human==1)){
   alpha::ProductionRetoolInput in{profile?alpha::Variant::alien_crossfire:alpha::Variant::base,load(b+4),load(b+8),load(b+12),human!=0,load(b+20),load(b+24),load(b+28),{}};memcpy(in.facilities,b+32,12);error=alpha::production_retool(in,output);
  }
  if(error!=alpha::LogicError::ok && (output.previous_class!=0x31415926 || output.queued_class!=0x31415926 || output.mineral_loss!=0x31415926)){fclose(f);return 21;}
  if(!emit((int32_t)error) || !emit(output.previous_class) || !emit(output.queued_class) || !emit(output.mineral_loss)){fclose(f);return 20;}
 }
}
