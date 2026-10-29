/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "air_port_combat.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[64];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[16];for(unsigned i=0;i<16;++i)x[i]=word(b+4*i);AirPortContext c{x[0],x[1],(uint32_t)x[2],(uint32_t)x[3],x[4]!=0,x[5]!=0,x[6]!=0,x[7]!=0,x[8],x[9],x[10],x[11]!=0,x[12],x[13],x[14],x[15]};unsigned char before[sizeof c];memcpy(before,&c,sizeof c);AirPortStrengths out;memset(&out,0xa5,sizeof out);unsigned char initial[sizeof out];memcpy(initial,&out,sizeof out);auto e=air_port_combat(c,out);unsigned guards=1;if(memcmp(before,&c,sizeof c))return 21;if(e!=LogicError::ok){if(memcmp(initial,&out,sizeof out))return 22;++guards;}if(air_port_combat(c,alias<AirPortStrengths>((uintptr_t)&c))!=LogicError::bad_state||memcmp(before,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)out.offense:0)||!emit(e==LogicError::ok?(uint32_t)out.defense:0)||!emit(e==LogicError::ok?out.applied:0)||!emit(guards))return 24;}}
