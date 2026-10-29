/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "artillery_combat.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[36];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[9];for(unsigned i=0;i<9;++i)x[i]=word(b+4*i);ArtilleryContext c{x[0],x[1],(uint32_t)x[2],(uint32_t)x[3],(uint32_t)x[4],(uint32_t)x[5],(uint32_t)x[6],x[7],x[8]};unsigned char before[sizeof c];memcpy(before,&c,sizeof c);ArtilleryStrengths out;memset(&out,0xa5,sizeof out);unsigned char initial[sizeof out];memcpy(initial,&out,sizeof out);auto e=artillery_combat(c,out);unsigned guards=1;if(memcmp(before,&c,sizeof c))return 21;if(e!=LogicError::ok){if(memcmp(initial,&out,sizeof out))return 22;++guards;}if(artillery_combat(c,alias<ArtilleryStrengths>((uintptr_t)&c))!=LogicError::bad_state||memcmp(before,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)out.offense:0)||!emit(e==LogicError::ok?(uint32_t)out.defense:0)||!emit(e==LogicError::ok?out.applied:0)||!emit(guards))return 24;}}
