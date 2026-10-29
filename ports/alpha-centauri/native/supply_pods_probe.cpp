/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "supply_pods.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static uint32_t word(const unsigned char*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t sign(uint32_t x){return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)~x;}
static bool emit(uint32_t x){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)x;x>>=8;}return fwrite(b,1,4,stdout)==4;}
int main(){unsigned char b[40];for(;;){const size_t n=fread(b,1,40,stdin);if(!n)return ferror(stdin)?20:0;if(n!=40)return 20;uint32_t v[10];for(unsigned i=0;i<10;++i)v[i]=word(b+4*i);OriginalMapInfo m{};m.width=v[0];m.height=v[1];m.random_seed=v[2];OriginalMapTile t{};t.climate=(uint8_t)v[3];t.items=v[4];PodCoefficients c{sign(v[8]),sign(v[9])};uint32_t out=0xa5a5a5a5;const auto e=tile_supply_pod(m,t,sign(v[5]),sign(v[6]),v[7],c,out);unsigned guards=0;if(e!=LogicError::ok&&out!=0xa5a5a5a5)return 21;
 const uintptr_t aliases[]={(uintptr_t)&m,(uintptr_t)&t,(uintptr_t)&c};for(auto p:aliases){if(tile_supply_pod(m,t,sign(v[5]),sign(v[6]),v[7],c,*reinterpret_cast<uint32_t*>(p))!=LogicError::bad_state)return 22;++guards;}
 if(!emit(e==LogicError::ok?0:1)||!emit(e==LogicError::ok?out:0)||!emit(guards))return 23;}}
