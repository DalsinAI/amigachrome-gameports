/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_prebattle_diplomacy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace alpha;
static bool get(uint32_t&n){unsigned char b[4];if(fread(b,1,4,stdin)!=4)return false;n=b[0]|uint32_t(b[1])<<8|uint32_t(b[2])<<16|uint32_t(b[3])<<24;return true;}
static void put(uint32_t n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};fwrite(b,1,4,stdout);}
template<class T>static T&at(uintptr_t p){return *reinterpret_cast<T*>(p);}
static uint32_t fingerprint(const OriginalWorld&w){uint32_t h=2166136261u;auto add=[&](const void*p,size_t n){const auto*b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;};const auto&g=w.game();add(w.original_bytes(),w.original_size());add(w.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile));add(w.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle));add(w.base(0),g.base_count*sizeof(OriginalBase));add(w.prototype(0),512*sizeof(OriginalPrototype));add(w.faction(0),8*sizeof(OriginalFactionState));add(w.faction_definition(0),8*sizeof(OriginalFactionDefinition));add(w.current_monuments().data,w.current_monuments().size);add(g.bunglist.data,g.bunglist.size);return h;}
int main(int argc,char**argv){if(argc!=2)return 20;FILE*f=fopen(argv[1],"rb");if(!f)return 20;if(fseek(f,0,SEEK_END))return 20;long n=ftell(f);rewind(f);if(n<1||n>64*1024*1024)return 20;auto*raw=(uint8_t*)malloc((size_t)n);if(!raw||fread(raw,1,(size_t)n,f)!=(size_t)n)return 20;fclose(f);OriginalWorld w;if(w.load(raw,(size_t)n,0x12345678)!=WorldLoadError::ok)return 20;const auto before=fingerprint(w);
 uint32_t words[9];while(get(words[0])){
 for(unsigned i=1;i<9;++i)if(!get(words[i]))return 21;
 const WorldPrebattleNotificationRuntime r{(BattleEntryObservation)words[2],words[3],words[4],words[5],words[6],words[7],words[8]};const auto prior_r=r;
 const WorldPrebattleAggressionRuntime ar{r.observation,r.current_faction};const auto prior_ar=ar;
 PrebattleNotificationProposal out;memset(&out,0xa5,sizeof out);const auto old=out;
 PrebattleAggressionProposal ao;memset(&ao,0xa5,sizeof ao);const auto old_ao=ao;
 const auto e=inspect_world_prebattle_notification(w,words[0],words[1],r,out);
 const auto ae=inspect_world_prebattle_aggression(w,words[0],words[1],ar,ao);
 if(e!=PrebattleNotificationError::ok&&memcmp(&old,&out,sizeof out))return 22;
 if(ae!=AggressionError::ok&&memcmp(&old_ao,&ao,sizeof ao))return 23;
 if(memcmp(&r,&prior_r,sizeof r)||memcmp(&ar,&prior_ar,sizeof ar))return 24;
 put((uint32_t)e);put((uint32_t)out.route);put(out.warning);put(out.spoke);
 put((uint32_t)ae);put(ao.forward_status);put(ao.reverse_status);put((uint32_t)ao.forward_combat_count);put((uint32_t)ao.reverse_combat_count);put(ao.combat_counts_reset);
 }
 WorldPrebattleNotificationRuntime r{BattleEntryObservation::supplied,3,2,128,1,99,7};
 WorldPrebattleAggressionRuntime ar{BattleEntryObservation::supplied,3};const auto guard_r=r;const auto guard_ar=ar;
 PrebattleNotificationProposal out{PrebattleNotificationRoute::observer,17,19};const auto prior=out;
 PrebattleAggressionProposal ao{11,13,17,19,0};const auto prior_ao=ao;unsigned guards=0;
 const uintptr_t addresses[]={(uintptr_t)&w,(uintptr_t)w.original_bytes(),(uintptr_t)w.tile(0),(uintptr_t)w.vehicle(0),(uintptr_t)w.base(0),(uintptr_t)w.prototype(0),(uintptr_t)w.faction(0),(uintptr_t)w.faction_definition(0),(uintptr_t)w.current_monuments().data,(uintptr_t)w.game().bunglist.data};
 for(auto address:addresses)if(address){const uintptr_t p=(address+3)&~uintptr_t(3);
  if(inspect_world_prebattle_notification(w,0,1,r,at<PrebattleNotificationProposal>(p))!=PrebattleNotificationError::bad_state)return 25;
  if(inspect_world_prebattle_aggression(w,0,1,ar,at<PrebattleAggressionProposal>(p))!=AggressionError::bad_state)return 26;
  if(inspect_world_prebattle_notification(w,0,1,at<WorldPrebattleNotificationRuntime>(p),out)!=PrebattleNotificationError::bad_state)return 27;
  if(inspect_world_prebattle_aggression(w,0,1,at<WorldPrebattleAggressionRuntime>(p),ao)!=AggressionError::bad_state)return 28;
  guards+=4;
 }
 for(unsigned offset=0;offset+sizeof out<=sizeof r;offset+=4){if(inspect_world_prebattle_notification(w,0,1,r,at<PrebattleNotificationProposal>((uintptr_t)&r+offset))!=PrebattleNotificationError::bad_state)return 29;++guards;}
 if(inspect_world_prebattle_aggression(w,0,1,ar,at<PrebattleAggressionProposal>((uintptr_t)&ar))!=AggressionError::bad_state)return 30;
 ++guards;
 if(inspect_world_prebattle_notification(w,0,1,at<WorldPrebattleNotificationRuntime>(4096),at<PrebattleNotificationProposal>(4096))!=PrebattleNotificationError::bad_state)return 31;
 if(inspect_world_prebattle_aggression(w,0,1,at<WorldPrebattleAggressionRuntime>(4096),at<PrebattleAggressionProposal>(4096))!=AggressionError::bad_state)return 32;
 guards+=2;
 OriginalWorld empty;
 if(inspect_world_prebattle_notification(empty,0,1,r,out)!=PrebattleNotificationError::bad_state||inspect_world_prebattle_aggression(empty,0,1,ar,ao)!=AggressionError::bad_state)return 33;
 guards+=2;
 if(memcmp(&r,&guard_r,sizeof r)||memcmp(&ar,&guard_ar,sizeof ar)||memcmp(&out,&prior,sizeof out)||memcmp(&ao,&prior_ao,sizeof ao)||fingerprint(w)!=before||w.runtime_seed()!=0x12345678||memcmp(w.original_bytes(),raw,(size_t)n))return 34;
 put(guards);free(raw);return ferror(stdin)||ferror(stdout)?35:0;}
