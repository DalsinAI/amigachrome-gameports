/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_rounds.h"
#include <stdio.h>
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool put(uint32_t v){uint8_t b[4];for(unsigned i=0;i<4;++i)b[i]=(uint8_t)(v>>(8*i));return fwrite(b,1,4,stdout)==4;}
// Deliberate invalid overlap references are rejected before dereference.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
static bool controls(){
 alpha::CombatDuel in{5,5,1,1,0,0};uint16_t draws[160]{};
 alpha::CombatRollStream stream{draws,160,0};alpha::CombatResult out;memset(&out,0x5a,sizeof out);
 const auto saved=out;
 if(alpha::resolve_combat_rounds(in,stream,*static_cast<alpha::CombatResult*>(static_cast<void*>(&stream)))!=alpha::CombatError::bad_buffer||stream.cursor)return false;
 if(alpha::resolve_combat_rounds(in,stream,*static_cast<alpha::CombatResult*>(static_cast<void*>(draws)))!=alpha::CombatError::bad_buffer||stream.cursor)return false;
 alpha::CombatRollStream nulls{nullptr,1,0};
 if(alpha::resolve_combat_rounds(in,nulls,out)!=alpha::CombatError::bad_buffer||memcmp(&out,&saved,sizeof out))return false;
 nulls={draws,1,2};
 if(alpha::resolve_combat_rounds(in,nulls,out)!=alpha::CombatError::bad_buffer||memcmp(&out,&saved,sizeof out))return false;
 alpha::CombatRollStream shorty{draws,19,0};
 if(alpha::resolve_combat_rounds(in,shorty,out)!=alpha::CombatError::short_random_stream||shorty.cursor||memcmp(&out,&saved,sizeof out))return false;
 alpha::CombatDuel bad=in;bad.attacker_damage=10;
 if(alpha::resolve_combat_rounds(bad,stream,out)!=alpha::CombatError::bad_state||stream.cursor||memcmp(&out,&saved,sizeof out))return false;
 return true;
}
#pragma GCC diagnostic pop
int main(){
 if(!controls())return 21;
 uint8_t row[352];
 while(true){const size_t n=fread(row,1,sizeof row,stdin);if(!n)break;if(n!=sizeof row)return 20;
  int32_t v[8];for(unsigned i=0;i<8;++i)v[i]=(int32_t)u32(row+4*i);
  if(v[6]<0||v[6]>160||v[7]<0)return 20;
  uint16_t rolls[160];for(unsigned i=0;i<160;++i)rolls[i]=(uint16_t)(row[32+2*i]|((uint16_t)row[33+2*i]<<8));
  alpha::CombatDuel duel{v[0],v[1],v[2],v[3],v[4],v[5]};
  alpha::CombatRollStream stream{rolls,(size_t)v[6],(size_t)v[7]};alpha::CombatResult result;memset(&result,0x5a,sizeof result);
  const auto before=result;auto e=alpha::resolve_combat_rounds(duel,stream,result);
  if(e!=alpha::CombatError::ok&&(memcmp(&before,&result,sizeof result)||stream.cursor!=(size_t)v[7]))return 21;
  uint32_t fields[11]={(uint32_t)e,(uint32_t)stream.cursor};
  if(e==alpha::CombatError::ok){fields[2]=(uint32_t)result.attacker_damage;fields[3]=(uint32_t)result.defender_damage;fields[4]=(uint32_t)result.attacker_hp;fields[5]=(uint32_t)result.defender_hp;fields[6]=result.rounds;fields[7]=result.attacker_hits;fields[8]=result.defender_hits;fields[9]=result.random_draws;fields[10]=(uint32_t)result.attacker_won;}
  else for(unsigned i=2;i<11;++i)fields[i]=0x5a5a5a5au;
  for(uint32_t field:fields)if(!put(field))return 20;
 }
 return ferror(stdin)||fflush(stdout)?20:0;
}
