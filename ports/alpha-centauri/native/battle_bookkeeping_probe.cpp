/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "battle_bookkeeping.h"
#include <stdio.h>
#include <string.h>
using namespace alpha;
static bool read32(uint32_t&v){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return true;}
static void write32(uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};fwrite(b,1,4,stdout);}
static int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:-1-(int32_t)(~v);}
static void readstate(const uint32_t*w,BattleBookkeepingState&s){size_t k=0;for(auto&v:s.wins)v=w[k++];for(auto&v:s.losses)v=w[k++];for(auto&r:s.combat_total)for(auto&v:r)v=w[k++];for(auto&r:s.combat_count)for(auto&v:r)v=w[k++];for(auto&r:s.enemy_destroyed){r.year=w[k++];r.opponent=w[k++];r.leader=w[k++];r.awarded=w[k++];r.rank=w[k++];r.focus=w[k++];}}
static void writestate(const BattleBookkeepingState&s){for(auto v:s.wins)write32(v);for(auto v:s.losses)write32(v);for(auto&r:s.combat_total)for(auto v:r)write32(v);for(auto&r:s.combat_count)for(auto v:r)write32(v);for(auto&r:s.enemy_destroyed){write32(r.year);write32(r.opponent);write32(r.leader);write32(r.awarded);write32(r.rank);write32(r.focus);}}
int main(){uint32_t w[210];while(read32(w[0])){for(size_t j=1;j<210;++j)if(!read32(w[j]))return 2;BattleBookkeepingContext c{(BookkeepingEdition)w[0],signed32(w[1]),signed32(w[2]),(BookkeepingOutcome)w[3],signed32(w[4]),w[5],w[6],(BookkeepingObservation)w[7],w[8],w[9],{}};for(size_t j=0;j<8;++j)c.rule_flags[j]=w[10+j];BattleBookkeepingState s;readstate(w+18,s);const auto original=s;BattleBookkeepingResult out;memset(&out,0xa5,sizeof out);const auto prior=out;auto e=preview_battle_bookkeeping(c,s,out);if(memcmp(&s,&original,sizeof s))return 3;if(e!=BookkeepingError::ok&&memcmp(&out,&prior,sizeof out))return 4;write32((uint32_t)e);writestate(out.before_cleanup);writestate(out.after_cleanup);write32(out.winner);write32(out.loser);write32(out.monument_changed);
 // Aligned overlap guards use valid live objects; no malformed reference UB.
 if(preview_battle_bookkeeping(c,out.before_cleanup,out)!=BookkeepingError::invalid)return 5;
 }return ferror(stdin)||ferror(stdout)?6:0;}
