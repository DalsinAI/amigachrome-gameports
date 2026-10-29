/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "combat_difficulty.h"
#include <cstdio>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
template<class T>static T&at(uintptr_t x){return *reinterpret_cast<T*>(x);}
int main(){uint32_t words[8];while(get(words[0])){for(unsigned i=1;i<8;++i)if(!get(words[i]))return 2;
 CombatDifficultyFacts f{(BattleEntryObservation)words[0],(Variant)words[1],words[2],words[3],words[4],signed_word(words[5]),signed_word(words[6]),signed_word(words[7])};const auto before=f;
 CombatDifficultyResult out;memset(&out,0xa5,sizeof out);const auto prior=out;auto error=inspect_combat_difficulty(f,out);
 if(error!=BattleEntryError::ok&&memcmp(&out,&prior,sizeof out))return 3;
 if(memcmp(&f,&before,sizeof f))return 4;
 put((uint32_t)error);put((uint32_t)out.offense);put(out.applied);
 if(inspect_combat_difficulty(f,at<CombatDifficultyResult>((uintptr_t)&f))!=BattleEntryError::bad_state||memcmp(&f,&before,sizeof f))return 5;
 if(inspect_combat_difficulty(at<CombatDifficultyFacts>(4096),at<CombatDifficultyResult>(4096))!=BattleEntryError::bad_state)return 6;
 }return ferror(stdin)||ferror(stdout)?7:0;}
