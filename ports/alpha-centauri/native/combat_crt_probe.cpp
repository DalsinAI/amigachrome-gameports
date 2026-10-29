/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_crt.h"
#include <stdio.h>
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool put(uint32_t n){uint8_t v[4];for(unsigned i=0;i<4;++i)v[i]=(uint8_t)(n>>(8*i));return fwrite(v,1,4,stdout)==4;}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
static bool controls(){
 alpha::CombatDuel in{5,5,1,1,0,0};alpha::CombatRandomState rng{1};alpha::CombatResult out;memset(&out,0x5a,sizeof out);
 const auto saved=out;
 if(alpha::resolve_seeded_combat_rounds(in,rng,*reinterpret_cast<alpha::CombatResult*>(&rng))!=alpha::CombatError::bad_buffer||rng.state!=1)return false;
 auto bad=in;bad.attacker_reactor=5;
 if(alpha::resolve_seeded_combat_rounds(bad,rng,out)!=alpha::CombatError::bad_state||rng.state!=1||memcmp(&out,&saved,sizeof out))return false;
 return true;
}
#pragma GCC diagnostic pop
int main(){
 if(!controls())return 21;
 uint8_t row[32];
 while(true){const size_t n=fread(row,1,sizeof row,stdin);if(!n)break;if(n!=sizeof row)return 20;
  uint32_t v[8];for(unsigned i=0;i<8;++i)v[i]=u32(row+4*i);
  alpha::CombatRandomState rng{v[0]};uint32_t out[11]{};
  if(v[1]){
   if(v[1]>65536)return 20;
   uint32_t h=2166136261u,first=0,last=0;for(uint32_t i=0;i<v[1];++i){last=alpha::combat_crt_draw(rng);if(!i)first=last;h=(h^last)*16777619u;}
   out[1]=rng.state;out[2]=first;out[3]=last;out[4]=h;out[5]=v[1];
  }else{
   alpha::CombatDuel in{(int32_t)v[2],(int32_t)v[3],(int32_t)v[4],(int32_t)v[5],(int32_t)v[6],(int32_t)v[7]};
   alpha::CombatResult result;memset(&result,0x5a,sizeof result);const auto saved=result;
   const auto e=alpha::resolve_seeded_combat_rounds(in,rng,result);out[0]=(uint32_t)e;out[1]=rng.state;
   if(e==alpha::CombatError::ok){out[2]=(uint32_t)result.attacker_damage;out[3]=(uint32_t)result.defender_damage;out[4]=(uint32_t)result.attacker_hp;out[5]=(uint32_t)result.defender_hp;out[6]=result.rounds;out[7]=result.attacker_hits;out[8]=result.defender_hits;out[9]=result.random_draws;out[10]=(uint32_t)result.attacker_won;}
   else {if(rng.state!=v[0]||memcmp(&saved,&result,sizeof result))return 21;for(unsigned i=2;i<11;++i)out[i]=0x5a5a5a5au;}
  }
  for(uint32_t word:out)if(!put(word))return 20;
 }
 return ferror(stdin)||fflush(stdout)?20:0;
}
