/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unit_availability.h"
#include <stdio.h>
#include <string.h>
static uint32_t word(const unsigned char* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t s(uint32_t n){int32_t v;memcpy(&v,&n,4);return v;}
static bool emit(uint32_t v){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;unsigned char b[72];size_t count=0;
 for(;;){size_t n=fread(b,1,72,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=72 || ++count>200000)return 20;uint32_t w[18];for(unsigned i=0;i<18;++i)w[i]=word(b+4*i);bool result=true;auto error=alpha::LogicError::bad_state;
 if(w[0]<=1 && w[3]<=65535 && w[4]<=255 && s(w[5])>=-128 && s(w[5])<=127 && w[6]<=255 && w[7]<=255 && w[10]<=1 && w[11]<=1 && w[12]<=255 && w[13]<=1 && s(w[14])>=-32768 && s(w[14])<=32767 && w[17]<=255){
 alpha::UnitAvailabilityInput in{w[0]?alpha::Variant::alien_crossfire:alpha::Variant::base,s(w[1]),s(w[2]),(uint16_t)w[3],(uint8_t)w[4],(int8_t)s(w[5]),(uint8_t)w[6],(uint8_t)w[7],w[8],w[9],w[10]!=0,w[11]!=0,(uint8_t)w[12],w[13]!=0,(int16_t)s(w[14]),s(w[15]),s(w[16]),(uint8_t)w[17]};error=alpha::unit_availability(in,result);}
 if(error!=alpha::LogicError::ok && !result)return 21;
 if(!emit((uint32_t)error) || !emit(result))return 20;
 }
}
