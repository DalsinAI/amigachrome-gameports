/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "native_combat_morale.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(){unsigned char b[36];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[9];for(unsigned i=0;i<9;++i)x[i]=word(b+4*i);const auto variant=x[0]==0?Variant::base:x[0]==1?Variant::alien_crossfire:(Variant)42;NativeCombatMoraleContext c{x[1],x[2],x[3],x[4],x[5],(uint32_t)x[6],(uint32_t)x[7],(uint32_t)x[8]};auto before=c;uint32_t out=0x12345678;auto e=native_combat_morale(variant,c,out);unsigned guards=0;if(memcmp(&before,&c,sizeof c))return 21;++guards;if(e!=LogicError::ok&&out!=0x12345678)return 22;if(e!=LogicError::ok)++guards;if(native_combat_morale(variant,c,c.state)!=LogicError::bad_state||memcmp(&before,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:1)||!emit(out)||!emit(guards))return 24;}}
