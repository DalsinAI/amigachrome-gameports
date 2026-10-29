/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_aggression.h"
#include <cstdio>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
template<class T>static T&at(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){uint32_t words[9];while(get(words[0])){for(unsigned i=1;i<9;++i)if(!get(words[i]))return 2;PrebattleAggressionFacts f{(BattleEntryObservation)words[0],(Variant)words[1],words[2],words[3],words[4],words[5],words[6],signed_word(words[7]),signed_word(words[8])};const auto before=f;PrebattleAggressionProposal out;memset(&out,0xa5,sizeof out);const auto prior=out;auto e=inspect_prebattle_aggression(f,out);
 if(e!=AggressionError::ok&&memcmp(&out,&prior,sizeof out))return 3;
 if(memcmp(&f,&before,sizeof f))return 4;
 put((uint32_t)e);put(out.forward_status);put(out.reverse_status);put((uint32_t)out.forward_combat_count);put((uint32_t)out.reverse_combat_count);put(out.combat_counts_reset);
 if(inspect_prebattle_aggression(f,at<PrebattleAggressionProposal>((uintptr_t)&f))!=AggressionError::bad_state||memcmp(&f,&before,sizeof f))return 5;
 if(inspect_prebattle_aggression(at<PrebattleAggressionFacts>(4096),at<PrebattleAggressionProposal>(4096))!=AggressionError::bad_state)return 6;
 if(e==AggressionError::ok){const auto original=out;auto again=f;again.forward_status=out.forward_status;again.reverse_status=out.reverse_status;again.forward_combat_count=out.forward_combat_count;again.reverse_combat_count=out.reverse_combat_count;PrebattleAggressionProposal second{};if(inspect_prebattle_aggression(again,second)!=AggressionError::ok||second.forward_status!=original.forward_status||second.reverse_status!=original.reverse_status||second.forward_combat_count!=original.forward_combat_count||second.reverse_combat_count!=original.reverse_combat_count||second.combat_counts_reset)return 7;}
 }return ferror(stdin)||ferror(stdout)?8:0;}
