/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "production_cost.h"
#include <stdio.h>
#include <string.h>
static int32_t word(const unsigned char* p){uint32_t n=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);int32_t s;memcpy(&s,&n,4);return s;}
static bool emit(int32_t n){uint32_t v=(uint32_t)n;unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)v;v>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(int argc,char** argv){if(argc!=2)return 20;FILE* f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char b[72];size_t count=0;for(;;){size_t n=fread(b,1,sizeof b,f);if(!n){bool ok=!ferror(f);fclose(f);return ok?0:20;}if(n!=sizeof b || ++count>200000){fclose(f);return 20;}
 int32_t r[18];for(unsigned i=0;i<18;++i)r[i]=word(b+i*4);
 alpha::UnitProductionCostResult out{0x31415926,0x31415926,true};alpha::LogicError e=alpha::LogicError::bad_state;
 bool valid=r[0]==0 || r[0]==1;for(unsigned i=8;i<=12;++i)valid=valid && (r[i]==0 || r[i]==1);
 if(valid){alpha::UnitProductionCostInput in{r[0]?alpha::Variant::alien_crossfire:alpha::Variant::base,r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8]!=0,r[9]!=0,r[10]!=0,r[11]!=0,r[12]!=0,r[13],r[14],r[15]};e=alpha::unit_production_cost(in,out);}
 if(e!=alpha::LogicError::ok && (out.cost!=0x31415926 || out.prototype_surcharge!=0x31415926 || !out.has_prototype_surcharge)){fclose(f);return 21;}
 int32_t requirement=0x31415926;auto q=alpha::production_mineral_requirement(r[16],r[17],requirement);
 if(q!=alpha::LogicError::ok && requirement!=0x31415926){fclose(f);return 21;}
 const int32_t values[]={(int32_t)e,out.cost,out.prototype_surcharge,(int32_t)out.has_prototype_surcharge,(int32_t)q,requirement};for(int32_t v:values)if(!emit(v)){fclose(f);return 20;}
 }}
