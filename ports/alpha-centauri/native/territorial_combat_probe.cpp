/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "territorial_combat.h"
#include <cstdio>
#include <cstring>
#ifdef __SANITIZE_ADDRESS__
#include <sanitizer/asan_interface.h>
#endif
using namespace alpha;
static bool read(uint32_t&v){v=0;for(unsigned j=0;j<4;++j){int c=getchar();if(c==EOF)return false;v|=(uint32_t)c<<(j*8);}return true;}
static void write(uint32_t v){for(unsigned j=0;j<4;++j){putchar((int)(v&255u));v>>=8;}}
static int32_t signed32(uint32_t v){return v<=0x7fffffffu?(int32_t)v:(int32_t)((int64_t)v-0x100000000ll);}
int main(){uint32_t v[13];unsigned aliases=0;while(read(v[0])){for(unsigned j=1;j<13;++j)if(!read(v[j]))return 2;
 alignas(TerritorialCombatResult) TerritorialCombatFacts f{TerritoryObservation::supplied,(Variant)v[0],v[1],signed32(v[2]),signed32(v[3]),signed32(v[4]),v[5],v[6],signed32(v[7]),signed32(v[8]),v[9],v[10],v[11]};f.observation=(TerritoryObservation)v[12];
 TerritorialCombatResult out;std::memset(&out,0x5a,sizeof out);const auto saved=out;const auto e=inspect_territorial_combat(f,out);write((uint32_t)e);
 if(e==TerritoryError::ok){write(out.applied);write((uint32_t)out.gift_before);write((uint32_t)out.gift_after);write((uint32_t)out.friction_before);write((uint32_t)out.friction_after);write(out.negotiation_before);write(out.negotiation_after);}else{if(std::memcmp(&out,&saved,sizeof out))return 3;for(unsigned j=0;j<7;++j)write(0);}
 const auto copy=f;const uintptr_t address=(uintptr_t)&f;
#ifdef __SANITIZE_ADDRESS__
 __asan_poison_memory_region(&f,sizeof f);
#endif
 const auto bad=inspect_territorial_combat(f,*reinterpret_cast<TerritorialCombatResult*>(address));
#ifdef __SANITIZE_ADDRESS__
 __asan_unpoison_memory_region(&f,sizeof f);
#endif
 if(bad!=TerritoryError::bad_state||std::memcmp(&copy,&f,sizeof f))return 4;
 ++aliases;
 }
 fprintf(stderr,"ALIASES %u\n",aliases);return ferror(stdin)?5:0;}
