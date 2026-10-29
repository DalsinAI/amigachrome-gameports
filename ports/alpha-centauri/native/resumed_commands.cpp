/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_session_data.h"
#include "initialized_snapshot.h"
#include <limits.h>
#include <new>
namespace alpha { namespace {
struct Buffer {uint8_t* p=nullptr;~Buffer(){free(p);}};
struct Table {Rules* p=nullptr;~Table(){free(p);}};
LocalOrderError world_error(WorldLoadError e){return e==WorldLoadError::memory_limit || e==WorldLoadError::out_of_memory?LocalOrderError::no_memory:LocalOrderError::bad_world;}
}
LocalOrderError LocalProductionSession::load_resumed_commands(const uint8_t*save,size_t n,const uint8_t*text,size_t nt,
 Variant profile,bool expansion,bool actions,uint32_t seed,size_t limit){
 if(!save || !n || !text || !nt || (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    (profile==Variant::base && expansion))return LocalOrderError::bad_state;
 if(actions && (profile!=Variant::base || expansion))return LocalOrderError::unsupported;
 if(!identity_ || revision_==UINT64_MAX)return LocalOrderError::overflow;
 Buffer normalized;int32_t human=-1;
 {
  if(limit<sizeof(Rules) || n>limit-sizeof(Rules))return LocalOrderError::no_memory;
  Table table;void*storage=malloc(sizeof(Rules));if(!storage)return LocalOrderError::no_memory;table.p=new(storage)Rules{};
  if(read_technologies(text,nt,*table.p)!=Error::ok)return LocalOrderError::bad_rules;
  OriginalWorld repaired;
  auto e=repaired.load_with_record_repairs(save,n,seed,*table.p,limit-sizeof(Rules)-n);
  if(e!=WorldLoadError::ok)return world_error(e);
  const auto&g=repaired.game();
  if((g.data_version<11)!=(profile==Variant::base))return LocalOrderError::unsupported;
  human=g.player;
  if(human<1 || human>7 || g.preferences.FactionStatus[0]!=(1u<<(unsigned)human))return LocalOrderError::not_human;
  if(!(g.preferences.FactionStatus[1]&(1u<<(unsigned)human)))return LocalOrderError::not_owner;
  const size_t used=sizeof(Rules)+n;
  if(repaired.memory_bytes()>limit-used)return LocalOrderError::no_memory;
  normalized.p=(uint8_t*)malloc(n);if(!normalized.p)return LocalOrderError::no_memory;
  LocalSnapshotReceipt receipt{};
  const auto encoded=encode_initialized_snapshot(repaired,*table.p,normalized.p,n,receipt,limit-used-repaired.memory_bytes());
  if(encoded!=LocalSnapshotError::ok)return encoded==LocalSnapshotError::out_of_memory?LocalOrderError::no_memory:LocalOrderError::bad_world;
  // Actual offline resume selects MapWin owner before entering commands.
  // This is a baseline selection, never upkeep or movement-budget refill.
  const size_t offset=12+(g.expanded_preferences?0x31c:0)+(g.expanded_preferences?0x390:0x314);
  if(offset>n || n-offset<4)return LocalOrderError::bad_world;
  for(unsigned i=0;i<4;++i)normalized.p[offset+i]=(uint8_t)((uint32_t)human>>(i*8));
  // Same observed command-dispatch prefix clears the PBEM-restart bit even
  // on this supported single-human offline path; preserve every other rule.
  const uint32_t more=(uint32_t)g.preferences.GameMoreRules&~0x40u;
  for(unsigned i=0;i<4;++i)normalized.p[offset-4+i]=(uint8_t)(more>>(i*8));
 }
 // The repair world and table are destroyed before the command session loads.
 // Do not replace the current session until all owning table imports succeed.
 LocalProductionSession candidate;
 const auto loaded=actions?candidate.load_local_worker_actions(normalized.p,n,text,nt,profile,expansion,human,seed,limit-n):
  candidate.load_local_accounting(normalized.p,n,text,nt,profile,expansion,human,seed,limit-n);
 if(loaded!=LocalOrderError::ok)return loaded;
 Data* previous=data_;data_=candidate.data_;candidate.data_=nullptr;
 ++revision_;suspended_=false;
 if(previous){previous->~Data();free(previous);}
 return LocalOrderError::ok;
}
}
