/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_combat.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[16];while(true){size_t n=fread(b,1,16,stdin);if(!n)return ferror(stdin)?20:0;if(n!=16)return 20;int32_t x[4];for(unsigned i=0;i<4;++i)x[i]=word(b+4*i);auto v=x[0]==0?Variant::base:x[0]==1?Variant::alien_crossfire:(Variant)42;SensorCombatAdjustment out;memset(&out,0xa5,sizeof out);unsigned char before[sizeof out];memcpy(before,&out,sizeof out);auto e=sensor_combat_adjustment(v,x[1],(uint32_t)x[2],x[3],out);if(e!=LogicError::ok&&memcmp(before,&out,sizeof out))return 21;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)out.defense:0)||!emit(e==LogicError::ok?out.coverage:0))return 22;}}
