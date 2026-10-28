/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_round.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static int32_t word(const unsigned char*p){uint32_t x=(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);return x<=0x7fffffffu?(int32_t)x:-1-(int32_t)(~x);}
static bool emit(uint32_t n){unsigned char b[4];for(unsigned i=0;i<4;++i){b[i]=(unsigned char)n;n>>=8;}return fwrite(b,1,4,stdout)==4;}
template<class T>static T&alias(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){unsigned char bytes[40];while(true){size_t n=fread(bytes,1,sizeof bytes,stdin);if(!n)return ferror(stdin)?20:0;if(n!=sizeof bytes)return 20;int32_t x[10];for(unsigned i=0;i<10;++i)x[i]=word(bytes+4*i);CombatRoundContext c{x[0],x[1],x[2]!=0,{(uint32_t)x[3],(uint32_t)x[4],(uint32_t)x[5]},{(uint32_t)x[6],(uint32_t)x[7],(uint32_t)x[8]},(uint32_t)x[9]};unsigned char before[sizeof c];memcpy(before,&c,sizeof c);CombatRoundResult out;memset(&out,0xa5,sizeof out);unsigned char initial[sizeof out];memcpy(initial,&out,sizeof out);auto e=combat_round_preview(c,out);unsigned guards=1;if(memcmp(before,&c,sizeof c))return 21;if(e!=LogicError::ok){if(memcmp(initial,&out,sizeof out))return 22;++guards;}if(combat_round_preview(c,alias<CombatRoundResult>((uintptr_t)&c))!=LogicError::bad_state||memcmp(before,&c,sizeof c))return 23;++guards;
 CombatHealth a{},b{};if(e==LogicError::ok&&(combat_health(c.attacker,a)!=LogicError::ok||combat_health(c.defender,b)!=LogicError::ok))return 24;
 const CombatHealthContext*profiles[]={&c.attacker,&c.defender};for(auto*p:profiles){CombatHealthContext ac=*p;unsigned char old[sizeof ac];memcpy(old,&ac,sizeof ac);if(combat_health(ac,alias<CombatHealth>((uintptr_t)&ac))!=LogicError::bad_state||memcmp(old,&ac,sizeof ac))return 25;++guards;}
 if(!emit(e==LogicError::ok?0:1))return 26;
 const uint32_t result[13]={out.next_crt_state,out.draw_count,out.attacker_roll,out.defender_roll,out.attacker_damage,out.defender_damage,out.attacker_health,out.defender_health,out.hit_side,a.power,b.power,a.remaining,b.remaining};for(auto value:result)if(!emit(e==LogicError::ok?value:0))return 26;if(!emit(guards))return 26;}}
