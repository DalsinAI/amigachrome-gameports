/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "human_upkeep_transaction.h"
#include <stdlib.h>
#include <string.h>
#include <new>
namespace alpha {
struct HumanUpkeepTransaction::Data {
 OriginalWorld source,candidate;
 Rules technologies;
 BasicRules basic;
 WorldRules world_rules;
 ResourceInfo resources;
 UnitDefinitions units;
 HumanUpkeepReceipt receipt{};
 uint32_t rules_crc=0;
 size_t world_limit=0;
};
namespace {
constexpr size_t header_size=64;
const uint8_t magic[8]={'A','C','H','U','P','K','0','1'};
uint32_t get(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
uint32_t extend(uint32_t c,const uint8_t*p,size_t n){while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320u:0);}return c;}
uint32_t crc(const uint8_t*p,size_t n){return ~extend(~0u,p,n);}
uint32_t envelope_crc(const uint8_t*p,size_t n){const uint8_t zero[4]{};return ~extend(extend(extend(~0u,p,32),zero,4),p+36,n-36);}
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool aliases(const OriginalWorld&w,const void*p,size_t n){
 if(!w.loaded())return overlap(p,n,&w,sizeof(w));
 return overlap(p,n,&w,sizeof(w))||overlap(p,n,w.original_bytes(),w.original_size())||
  overlap(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase))||
  overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||
  overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
HumanUpkeepError translate(StableIntakeError e){switch(e){
 case StableIntakeError::ok:return HumanUpkeepError::ok;
 case StableIntakeError::bad_buffer:return HumanUpkeepError::bad_buffer;
 case StableIntakeError::overflow:return HumanUpkeepError::overflow;
 case StableIntakeError::unsupported_stage:case StableIntakeError::needs_allocation:return HumanUpkeepError::unsupported;
 case StableIntakeError::out_of_memory:return HumanUpkeepError::no_memory;
 default:return HumanUpkeepError::bad_world;
}}
HumanUpkeepError translate(WorldLoadError e){return e==WorldLoadError::memory_limit||e==WorldLoadError::out_of_memory?HumanUpkeepError::no_memory:HumanUpkeepError::bad_world;}
}
void HumanUpkeepTransaction::close(){if(data_){data_->~Data();free(data_);data_=nullptr;}}
const OriginalWorld* HumanUpkeepTransaction::source_world()const{return data_?&data_->source:nullptr;}
const OriginalWorld* HumanUpkeepTransaction::proposed_world()const{return data_?(data_->candidate.loaded()?&data_->candidate:&data_->source):nullptr;}
const HumanUpkeepReceipt* HumanUpkeepTransaction::receipt()const{return data_&&data_->receipt.phase!=HumanUpkeepPhase::unprepared?&data_->receipt:nullptr;}
HumanUpkeepPhase HumanUpkeepTransaction::phase()const{return data_?data_->receipt.phase:HumanUpkeepPhase::unprepared;}
HumanUpkeepError HumanUpkeepTransaction::start(const uint8_t* bytes,size_t n,const uint8_t* rules,size_t rn,uint32_t seed,size_t limit){
 if(data_||!bytes||!n||!rules||!rn)return HumanUpkeepError::bad_state;
 if(limit<sizeof(Data))return HumanUpkeepError::no_memory;
 void* storage=malloc(sizeof(Data));if(!storage)return HumanUpkeepError::no_memory;
 auto* next=new(storage)Data;
 next->world_limit=(limit-sizeof(Data))/2+sizeof(OriginalWorld);
 HumanUpkeepError error=HumanUpkeepError::ok;
 if(read_technologies(rules,rn,next->technologies)!=Error::ok ||
    read_basic_rules(rules,rn,next->technologies,Variant::base,next->basic,next->world_rules)!=Error::ok ||
    read_resource_info(rules,rn,next->resources)!=Error::ok ||
    read_unit_definitions(rules,rn,next->technologies,Variant::base,next->units)!=Error::ok)error=HumanUpkeepError::bad_rules;
 if(error==HumanUpkeepError::ok){const auto loaded=next->source.load(bytes,n,seed,next->world_limit);if(loaded!=WorldLoadError::ok)error=translate(loaded);}
 if(error==HumanUpkeepError::ok){const auto&g=next->source.game();const int32_t human=g.player;
  if(g.data_version>=11||human<1||human>7||!(g.preferences.FactionStatus[0]&(1u<<(unsigned)human))||g.preferences.CurrentFaction!=human)error=HumanUpkeepError::unsupported;
 }
 if(error!=HumanUpkeepError::ok){next->~Data();free(next);return error;}
 next->rules_crc=crc(rules,rn);data_=next;return HumanUpkeepError::ok;
}
HumanUpkeepError HumanUpkeepTransaction::prepare_population(size_t id,bool full_turn){
 if(!data_)return HumanUpkeepError::bad_state;
 if(data_->receipt.phase!=HumanUpkeepPhase::unprepared)return HumanUpkeepError::already_prepared;
 const auto& source=data_->source;const auto*base=source.base(id);
 if(!base||base->faction_id!=source.game().player)return HumanUpkeepError::bad_state;
 const WorldPopulationStage stage{{Variant::base,false,false,{0,false},0,0},full_turn};
 HumanUpkeepReceipt result{};result.base=id;result.full_turn=full_turn;
 auto error=world_population_begin(source,data_->technologies,data_->basic,data_->resources,id,stage,result.population);
 if(error!=StableIntakeError::ok)return translate(error);
 OriginalWorld candidate;
 const auto loaded=candidate.load(source.original_bytes(),source.original_size(),source.runtime_seed(),data_->world_limit);
 if(loaded!=WorldLoadError::ok)return translate(loaded);
 if(result.population.transition.removed){
  result.phase=HumanUpkeepPhase::base_removal_pending;
 }else{
  const auto&transition=result.population.transition;
  if(transition.population<1||transition.population>127)return HumanUpkeepError::unsupported;
  auto&next=candidate.bases_[id];next.pop_size=(int8_t)transition.population;next.nutrients_accumulated=transition.accumulated;
  for(size_t i=0;i<candidate.game().vehicle_count;++i)if(result.population.convoy_cancellations[i/32]&(1u<<(i%32)))candidate.vehicles_[i].order=0;
  const WorkerUpkeepStage worker_stage{1,false,false};
  error=world_worker_upkeep_candidate(candidate,data_->technologies,data_->units,data_->basic,data_->resources,id,worker_stage,result.workers);
  if(error!=StableIntakeError::ok)return translate(error);
  const auto&assignment=result.workers.allocation;
  next.worked_tiles=(int32_t)assignment.worked;
  next.specialist_total=assignment.specialists;
  result.specialists_pending=assignment.requires_specialist_resolution||assignment.unallocated||assignment.specialists;
  result.riots_pending=assignment.requires_riot_resolution;
  // A valid provisional no-specialist mask permits a numeric fresh-food check,
  // while specialist optimisation/riots remain unresolved blocking events.
  if(!assignment.specialists&&!assignment.unallocated){
   if(result.population.recompute_required){
    WorldPopulationProposal finished{};
    error=world_population_finish(source,candidate,data_->technologies,data_->basic,data_->resources,id,stage,finished);
    if(error!=StableIntakeError::ok)return translate(error);
    result.population=finished;next.nutrients_accumulated=finished.transition.accumulated;
   }
   result.food_checked=true;
  }
  result.phase=result.specialists_pending?HumanUpkeepPhase::specialists_pending:result.riots_pending?HumanUpkeepPhase::riots_pending:HumanUpkeepPhase::population_checked;
 }
 // Qualify serialization before retaining any private phase result.
 auto* serialized=(uint8_t*)malloc(source.original_size());
 if(!serialized)return HumanUpkeepError::no_memory;
 LocalSnapshotReceipt encoded{};
 const auto snapshot=encode_human_upkeep_snapshot(candidate,serialized,source.original_size(),encoded);free(serialized);
 if(snapshot!=LocalSnapshotError::ok)return snapshot==LocalSnapshotError::out_of_memory?HumanUpkeepError::no_memory:HumanUpkeepError::unsupported;
 // Every computation and all possible failures precede this private swap.
 data_->candidate.swap(candidate);data_->receipt=result;return HumanUpkeepError::ok;
}
size_t HumanUpkeepTransaction::checkpoint_size()const{
 if(!data_)return 0;
 const size_t n=data_->source.original_size();
 return n<=UINT32_MAX&&n<=(SIZE_MAX-header_size)/2?header_size+2*n:0;
}
LocalSnapshotError HumanUpkeepTransaction::checkpoint(void* output,size_t capacity)const{
 if(!data_)return LocalSnapshotError::bad_world;
 const size_t total=checkpoint_size(),n=data_->source.original_size();
 if(!total||!output||capacity<total||overlap(output,total,this,sizeof(*this))||overlap(output,total,data_,sizeof(Data))||
    aliases(data_->source,output,total)||aliases(data_->candidate,output,total))return LocalSnapshotError::bad_buffer;
 auto* tmp=(uint8_t*)calloc(1,total);if(!tmp)return LocalSnapshotError::out_of_memory;
 memcpy(tmp+header_size,data_->source.original_bytes(),n);
 LocalSnapshotReceipt encoded{};
 auto error=LocalSnapshotError::ok;
 if(data_->candidate.loaded())error=encode_human_upkeep_snapshot(data_->candidate,tmp+header_size+n,n,encoded);
 else memcpy(tmp+header_size+n,data_->source.original_bytes(),n);
 if(error==LocalSnapshotError::ok){
  memcpy(tmp,magic,8);put(tmp+8,header_size);put(tmp+12,(uint32_t)n);put(tmp+16,data_->source.runtime_seed());put(tmp+20,data_->rules_crc);
  put(tmp+24,crc(tmp+header_size,n));put(tmp+28,crc(tmp+header_size+n,n));
  put(tmp+36,receipt()?(uint32_t)data_->receipt.base:UINT32_MAX);put(tmp+40,(uint32_t)data_->receipt.full_turn);put(tmp+44,(uint32_t)data_->receipt.phase);
  put(tmp+32,envelope_crc(tmp,total));memcpy(output,tmp,total);
 }
 free(tmp);return error;
}
HumanUpkeepError HumanUpkeepTransaction::resume(const uint8_t* input,size_t n,const uint8_t* rules,size_t rn,size_t limit){
 if(!input||n<header_size||memcmp(input,magic,8)||get(input+8)!=header_size||
    (n-header_size)%2||get(input+12)!=(n-header_size)/2||get(input+32)!=envelope_crc(input,n)||get(input+40)>1)return HumanUpkeepError::bad_checkpoint;
 const size_t world_size=get(input+12);
 if(get(input+24)!=crc(input+header_size,world_size)||get(input+28)!=crc(input+header_size+world_size,world_size))return HumanUpkeepError::bad_checkpoint;
 for(unsigned i=48;i<64;++i)if(input[i])return HumanUpkeepError::bad_checkpoint;
 if(!rules||!rn||get(input+20)!=crc(rules,rn))return HumanUpkeepError::bad_rules;
 HumanUpkeepTransaction restored;
 auto error=restored.start(input+header_size,world_size,rules,rn,get(input+16),limit);if(error!=HumanUpkeepError::ok)return error;
 if(get(input+44)==(uint32_t)HumanUpkeepPhase::unprepared){
  if(get(input+36)!=UINT32_MAX||get(input+40))return HumanUpkeepError::bad_checkpoint;
 }else{
  error=restored.prepare_population(get(input+36),get(input+40)!=0);
  if(error==HumanUpkeepError::no_memory)return error;
  if(error!=HumanUpkeepError::ok)return HumanUpkeepError::bad_checkpoint;
 }
 // Rebuild the actual private mutations/events from owned source and rules.
 // A checksum-valid caller phase, worker mask, credit total or stored-food
 // edit cannot advance an unresolved event or invent a qualified stage result.
 auto* expected=(uint8_t*)malloc(n);if(!expected)return HumanUpkeepError::no_memory;
 const auto encoded=restored.checkpoint(expected,n);
 const bool same=encoded==LocalSnapshotError::ok&&!memcmp(expected,input,n);free(expected);
 if(!same)return HumanUpkeepError::bad_checkpoint;
 Data* old=data_;data_=restored.data_;restored.data_=old;return HumanUpkeepError::ok;
}
}
