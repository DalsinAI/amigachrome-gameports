/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "attack_modifier_ledger.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static bool get(uint32_t&v){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return true;}
static bool put(uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};return fwrite(b,1,4,stdout)==4;}
static int32_t si(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
template<class T>T&at(uintptr_t n){return *reinterpret_cast<T*>(n);}
int main(){uint32_t x[27];while(get(x[0])){
 for(unsigned i=1;i<27;++i)if(!get(x[i]))return 2;
 AttackModifierFacts f{(BattleEntryObservation)x[0],(Variant)x[1],x[2],x[3],si(x[4]),x[5],{},x[14],si(x[15]),si(x[16]),si(x[17])};for(unsigned i=0;i<8;++i)f.offense_factors[i]=si(x[6+i]);AttackModifierLedger prior{x[18],{}};for(unsigned i=0;i<4;++i)prior.slots[i]={x[19+i*2],si(x[20+i*2])};
 unsigned char saved_f[sizeof f],saved_prior[sizeof prior];memcpy(saved_f,&f,sizeof f);memcpy(saved_prior,&prior,sizeof prior);
 AttackModifierResult out;memset(&out,0xa5,sizeof out);unsigned char before[sizeof out];memcpy(before,&out,sizeof out);auto e=attack_modifier_ledger(f,prior,out);if(e!=BattleEntryError::ok&&memcmp(before,&out,sizeof out))return 3;
 if(attack_modifier_ledger(f,prior,at<AttackModifierResult>((uintptr_t)&f))!=BattleEntryError::bad_state||attack_modifier_ledger(f,prior,at<AttackModifierResult>((uintptr_t)&prior))!=BattleEntryError::bad_state)return 4;
 if(attack_modifier_ledger(at<AttackModifierFacts>(4096),prior,at<AttackModifierResult>(4096))!=BattleEntryError::bad_state||attack_modifier_ledger(f,at<AttackModifierLedger>(4096),at<AttackModifierResult>(4096))!=BattleEntryError::bad_state)return 5;
 if(attack_modifier_ledger(f,at<AttackModifierLedger>((uintptr_t)&f),out)!=BattleEntryError::bad_state)return 9;
 if(memcmp(saved_f,&f,sizeof f)||memcmp(saved_prior,&prior,sizeof prior))return 8;
 if(!put((uint32_t)e)||!put(out.ledger.count))return 6;
 for(auto&s:out.ledger.slots)if(!put(s.label)||!put((uint32_t)s.percent))return 6;
 if(!put(out.lookup_count))return 6;
 for(auto&v:out.lookups)if(!put(v.label)||!put((uint32_t)v.percent)||!put(v.appended))return 6;
 }return ferror(stdin)||fflush(stdout)?7:0;}
