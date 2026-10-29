/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "prebattle_territory.h"
#include <cstdio>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
template<class T>static T&at(uintptr_t p){return *reinterpret_cast<T*>(p);}
int main(){uint32_t w[15];while(get(w[0])){
 for(unsigned i=1;i<15;++i)if(!get(w[i]))return 2;
 const PrebattleTerritoryFacts f{(BattleEntryObservation)w[0],(Variant)w[1],w[2],w[3],w[4],signed_word(w[5]),w[6],w[7],w[8],signed_word(w[9]),signed_word(w[10]),signed_word(w[11]),w[12],w[13],w[14]};const auto before=f;
 PrebattleTerritoryProposal out;memset(&out,0xa5,sizeof out);const auto prior=out;const auto e=inspect_prebattle_territory(f,out);
 if(e!=PrebattleTerritoryError::ok&&memcmp(&out,&prior,sizeof out))return 3;
 if(memcmp(&f,&before,sizeof f))return 4;
 put((uint32_t)e);put(out.owner_defender_status);put(out.defender_owner_status);put((uint32_t)out.defender_owner_friction);put(out.negotiation_accumulator);put(out.treaty_applied);put(out.friction_applied);
 for(unsigned offset=0;offset+sizeof out<=sizeof f;offset+=4){if(inspect_prebattle_territory(f,at<PrebattleTerritoryProposal>((uintptr_t)&f+offset))!=PrebattleTerritoryError::bad_state||memcmp(&f,&before,sizeof f))return 5;}
 if(inspect_prebattle_territory(at<PrebattleTerritoryFacts>(4096),at<PrebattleTerritoryProposal>(4096))!=PrebattleTerritoryError::bad_state)return 6;
 }return ferror(stdin)||ferror(stdout)?7:0;}
