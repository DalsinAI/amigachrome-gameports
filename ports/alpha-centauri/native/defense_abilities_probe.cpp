/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "defense_abilities.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[40];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[10];for(unsigned i=0;i<10;++i)x[i]=word(b+4*i);DefenseAbilitiesContext c{(Variant)x[0],x[1],(uint32_t)x[2],(uint32_t)x[3],(uint32_t)x[4],x[5]!=0,x[6]!=0,x[7]!=0,x[8],x[9]};unsigned char before[sizeof c];memcpy(before,&c,sizeof c);DefenseAbilitiesStrength out;memset(&out,0xa5,sizeof out);unsigned char initial[sizeof out];memcpy(initial,&out,sizeof out);auto e=defense_abilities(c,out);unsigned guards=1;if(memcmp(before,&c,sizeof c))return 21;if(e!=LogicError::ok){if(memcmp(initial,&out,sizeof out))return 22;++guards;}if(defense_abilities(c,alias<DefenseAbilitiesStrength>((uintptr_t)&c))!=LogicError::bad_state||memcmp(before,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)out.defense:0)||!emit(e==LogicError::ok?out.applied:0)||!emit(guards))return 24;}}
