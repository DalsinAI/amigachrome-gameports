/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_difficulty.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
static int32_t signed_word(uint32_t n){return n&0x80000000u?-(int32_t)(~n)-1:(int32_t)n;}
template<class T>static T&at(uintptr_t p){return *reinterpret_cast<T*>(p);}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;auto add=[&](const void*p,size_t n){const auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};const auto&g=w.game();add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle));add(w.base(0),g.base_count*sizeof(OriginalBase));add(w.prototype(0),512*sizeof(OriginalPrototype));add(w.faction(0),8*sizeof(OriginalFactionState));add(w.faction_definition(0),8*sizeof(OriginalFactionDefinition));add(w.current_monuments().data,w.current_monuments().size);add(g.bunglist.data,g.bunglist.size);return h;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END))return 20;long n=ftell(f);rewind(f);if(n<1||n>64*1024*1024)return 20;auto*raw=(uint8_t*)malloc((size_t)n);if(!raw||fread(raw,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);OriginalWorld w;if(w.load(raw,(size_t)n,0x12345678)!=WorldLoadError::ok)return 20;const auto before=fingerprint(w);
 uint32_t words[5];while(get(words[0])){for(unsigned i=1;i<5;++i)if(!get(words[i]))return 21;const WorldCombatDifficultyRuntime runtime{(BattleEntryObservation)words[2],words[3],signed_word(words[4])};const auto runtime_before=runtime;CombatDifficultyResult out;memset(&out,0xa5,sizeof out);const auto old=out;auto e=inspect_world_combat_difficulty(w,words[0],words[1],runtime,out);if(e!=BattleEntryError::ok&&memcmp(&old,&out,sizeof out))return 22;if(memcmp(&runtime,&runtime_before,sizeof runtime))return 23;put((uint32_t)e);put((uint32_t)out.offense);put(out.applied);}
 WorldCombatDifficultyRuntime runtime{BattleEntryObservation::supplied,2,100};CombatDifficultyResult out{17,19};const auto original=out;unsigned guards=0;
 const uintptr_t addresses[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)w.current_monuments().data,(uintptr_t)w.game().bunglist.data,(uintptr_t)&runtime};
 for(auto p:addresses)if(p){p=(p+alignof(CombatDifficultyResult)-1)&~uintptr_t(alignof(CombatDifficultyResult)-1);if(inspect_world_combat_difficulty(w,0,1,runtime,at<CombatDifficultyResult>(p))!=BattleEntryError::bad_state)return 24;++guards;}
 if(inspect_world_combat_difficulty(w,0,1,at<WorldCombatDifficultyRuntime>(4096),at<CombatDifficultyResult>(4096))!=BattleEntryError::bad_state)return 25;
 ++guards;
 for(auto p:addresses)if(p&&p!=(uintptr_t)&runtime){p=(p+alignof(WorldCombatDifficultyRuntime)-1)&~uintptr_t(alignof(WorldCombatDifficultyRuntime)-1);if(inspect_world_combat_difficulty(w,0,1,at<WorldCombatDifficultyRuntime>(p),out)!=BattleEntryError::bad_state||memcmp(&out,&original,sizeof out))return 26;++guards;}
 OriginalWorld empty;if(inspect_world_combat_difficulty(empty,0,1,runtime,out)!=BattleEntryError::bad_state||memcmp(&out,&original,sizeof out))return 27;++guards;
 if(fingerprint(w)!=before||w.runtime_seed()!=0x12345678||memcmp(w.original_bytes(),raw,(size_t)n))return 28;
 put(guards);free(raw);return ferror(stdin)||ferror(stdout)?29:0;}
