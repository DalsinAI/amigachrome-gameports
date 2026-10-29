/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "base_combat.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[76];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[19];for(unsigned i=0;i<19;++i)x[i]=word(b+4*i);const auto v=x[0]==0?Variant::base:x[0]==1?Variant::alien_crossfire:(Variant)42;BaseCombatContext c{x[1],x[2],x[3]!=0,x[4],x[5],x[6],x[7],x[8],x[9]!=0,x[10]!=0,x[11]!=0,x[12]!=0,x[13]!=0,x[14]!=0,x[15]!=0,x[16]!=0,x[17],x[18]};unsigned char source[sizeof c];memcpy(source,&c,sizeof c);BaseCombatAdjustment out;memset(&out,0xa5,sizeof out);unsigned char original[sizeof out];memcpy(original,&out,sizeof out);auto e=base_combat_adjustment(v,c,out);if(memcmp(source,&c,sizeof c))return 21;unsigned guards=1;if(e!=LogicError::ok){if(memcmp(original,&out,sizeof out))return 22;++guards;}if(base_combat_adjustment(v,c,alias<BaseCombatAdjustment>((uintptr_t)&c))!=LogicError::bad_state||memcmp(source,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:e==LogicError::overflow?2:1)||!emit(e==LogicError::ok?(uint32_t)out.offense:0)||!emit(e==LogicError::ok?(uint32_t)out.defense:0)||!emit(e==LogicError::ok?out.multiplier:0)||!emit(e==LogicError::ok?out.base_percent_applied:0)||!emit(e==LogicError::ok?out.infantry_percent_applied:0)||!emit(guards))return 24;}}
