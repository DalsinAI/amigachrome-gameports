/* SPDX-License-Identifier: GPL-3.0-or-later; source attribution combat_rounds.h. */
#include "combat_rounds.h"
namespace alpha {namespace {
bool overlaps(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool take(CombatRollStream&s,int32_t strength,int32_t&value){
 value=0;if(strength<=1)return true;
 if(s.cursor>=s.count||s.values[s.cursor]>32767)return false;
 value=s.values[s.cursor++]%(uint32_t)strength;return true;
}
}
CombatError resolve_combat_rounds(const CombatDuel&in,CombatRollStream&stream,CombatResult&out){
 if(stream.count>SIZE_MAX/sizeof(uint16_t))return CombatError::bad_buffer;
 const size_t bytes=stream.count*sizeof(uint16_t);
 if(overlaps(&out,sizeof out,&in,sizeof in)||overlaps(&out,sizeof out,&stream,sizeof stream)
 ||overlaps(&stream,sizeof stream,&in,sizeof in)
 ||overlaps(&out,sizeof out,stream.values,bytes)||overlaps(&stream,sizeof stream,stream.values,bytes))return CombatError::bad_buffer;
 if(stream.cursor>stream.count||(!stream.values&&stream.count))return CombatError::bad_buffer;
 if(in.offense<0||in.defense<0||in.attacker_reactor<1||in.attacker_reactor>4
 ||in.defender_reactor<1||in.defender_reactor>4||in.attacker_damage<0||in.defender_damage<0
 ||in.attacker_damage>=10*in.attacker_reactor||in.defender_damage>=10*in.defender_reactor)return CombatError::bad_state;
 CombatRollStream next=stream;CombatResult result{};
 result.attacker_damage=in.attacker_damage;result.defender_damage=in.defender_damage;
 // At most 79 lost hitpoints: every iteration damages exactly one living unit.
 while(result.attacker_damage<10*in.attacker_reactor&&result.defender_damage<10*in.defender_reactor){
  int32_t attack,defend;
  if(!take(next,in.offense,attack)||!take(next,in.defense,defend))return CombatError::short_random_stream;
  if(attack<=defend){++result.attacker_damage;++result.defender_hits;}
  else {++result.defender_damage;++result.attacker_hits;}
  ++result.rounds;
 }
 result.attacker_hp=10*in.attacker_reactor-result.attacker_damage;
 result.defender_hp=10*in.defender_reactor-result.defender_damage;
 result.attacker_won=result.defender_hp==0;
 result.random_draws=(uint32_t)(next.cursor-stream.cursor);
 stream.cursor=next.cursor;out=result;return CombatError::ok;
}
const char* combat_error_name(CombatError e){switch(e){
 case CombatError::ok:return "ok";case CombatError::bad_state:return "bad_state";
 case CombatError::bad_buffer:return "bad_buffer";case CombatError::unsupported:return "unsupported";
 case CombatError::overflow:return "overflow";case CombatError::short_random_stream:return "short_random_stream";
 }return "unknown";}
}
