/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_defense.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char b[56];while(true){size_t n=fread(b,1,sizeof b,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof b)return 20;int32_t x[14];for(unsigned i=0;i<14;++i)x[i]=word(b+4*i);TerrainDefenseContext c{(uint32_t)x[0],(uint32_t)x[1],(uint32_t)x[2],(uint32_t)x[3],x[4],x[5],x[6],x[7],x[8]!=0,x[9],x[10],x[11],x[12],x[13]};unsigned char source[sizeof c];memcpy(source,&c,sizeof c);TerrainDefense out;memset(&out,0xa5,sizeof out);unsigned char initial[sizeof out];memcpy(initial,&out,sizeof out);auto e=terrain_defense(c,out);if(memcmp(source,&c,sizeof c))return 21;unsigned guards=1;if(e!=LogicError::ok){if(memcmp(initial,&out,sizeof out))return 22;++guards;}if(terrain_defense(c,alias<TerrainDefense>((uintptr_t)&c))!=LogicError::bad_state||memcmp(source,&c,sizeof c))return 23;++guards;if(!emit(e==LogicError::ok?0:1)||!emit(e==LogicError::ok?out.multiplier:0)||!emit(e==LogicError::ok?(uint32_t)out.label:0)||!emit(guards))return 24;}}
