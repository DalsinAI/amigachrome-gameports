/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "facility_upkeep_session.h"
#include <stdlib.h>
#include <string.h>
#include <new>
namespace alpha {
struct FacilityUpkeepSession::Data {
 OriginalWorld world;
 Rules technologies;
 BasicRules basic;
 WorldRules world_rules;
 ResourceInfo resources;
 FacilityDefinitions facilities;
 UnitDefinitions units;
 uint32_t rules_crc=0;
 uint8_t processed[64]{},recompute[64]{},choice[64]{};
};
namespace {
constexpr size_t header_size=256;
const uint8_t magic[8]={'A','C','F','U','P','K','0','1'};
uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
void put(uint8_t* p,uint32_t n){for(unsigned i=0;i<4;++i){p[i]=(uint8_t)n;n>>=8;}}
uint32_t extend(uint32_t c,const uint8_t* p,size_t n){while(n--){c^=*p++;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320u:0);}return c;}
uint32_t crc(const uint8_t* p,size_t n){return ~extend(~0u,p,n);}
uint32_t checkpoint_crc(const uint8_t* p,size_t n){
 const uint8_t zeros[4]{};
 return ~extend(extend(extend(~0u,p,28),zeros,4),p+32,n-32);
}
bool bit(const uint8_t* p,size_t i){return i<512 && (p[i/8]&(1u<<(i%8)));}
void set(uint8_t* p,size_t i){p[i/8]|=(uint8_t)(1u<<(i%8));}
bool overlap(const void* a,size_t n,const void* b,size_t m){
 if(!n || !m)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 return n>UINTPTR_MAX-x || m>UINTPTR_MAX-y || (x<y+m && y<x+n);
}
bool aliases(const OriginalWorld& w,const void* p,size_t n){
 return overlap(p,n,&w,sizeof(w)) || overlap(p,n,w.original_bytes(),w.original_size()) ||
  overlap(p,n,w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) ||
  overlap(p,n,w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) ||
  overlap(p,n,w.base(0),w.game().base_count*sizeof(OriginalBase)) ||
  overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype)) ||
  overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState)) ||
  overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
void FacilityUpkeepSession::close(){if(data_){data_->~Data();free(data_);data_=nullptr;}}
const OriginalWorld* FacilityUpkeepSession::world()const{return data_?&data_->world:nullptr;}
bool FacilityUpkeepSession::processed(size_t i)const{return data_ && bit(data_->processed,i);}
bool FacilityUpkeepSession::recompute_pending(size_t i)const{return data_ && bit(data_->recompute,i);}
bool FacilityUpkeepSession::production_choice_pending(size_t i)const{return data_ && bit(data_->choice,i);}
UpkeepSessionError FacilityUpkeepSession::prepare(const uint8_t* save,size_t n,const uint8_t* rules,size_t rn,uint32_t seed,size_t limit,Data*& out){
 if(!save || !rules || !n || !rn)return UpkeepSessionError::bad_state;
 if(limit<sizeof(Data))return UpkeepSessionError::no_memory;
 void* storage=malloc(sizeof(Data));if(!storage)return UpkeepSessionError::no_memory;
 auto* next=new(storage)Data;
 UpkeepSessionError error=UpkeepSessionError::ok;
 if(read_technologies(rules,rn,next->technologies)!=Error::ok ||
    read_basic_rules(rules,rn,next->technologies,Variant::base,next->basic,next->world_rules)!=Error::ok ||
    read_facility_definitions(rules,rn,next->technologies,Variant::base,next->facilities)!=Error::ok ||
    read_unit_definitions(rules,rn,next->technologies,Variant::base,next->units)!=Error::ok ||
    read_resource_info(rules,rn,next->resources)!=Error::ok)error=UpkeepSessionError::bad_rules;
 if(error==UpkeepSessionError::ok){
  const auto loaded=next->world.load(save,n,seed,limit-sizeof(Data)+sizeof(OriginalWorld));
  if(loaded!=WorldLoadError::ok)error=(loaded==WorldLoadError::out_of_memory || loaded==WorldLoadError::memory_limit)?UpkeepSessionError::no_memory:UpkeepSessionError::bad_world;
 }
 if(error==UpkeepSessionError::ok){
  const auto& game=next->world.game();const int32_t human=game.player;
  if(game.data_version>=11 || game.base_count>512 || human<1 || human>7 ||
     !(game.preferences.FactionStatus[0]&(1u<<(unsigned)human)) || game.preferences.CurrentFaction!=human)error=UpkeepSessionError::bad_world;
 }
 if(error!=UpkeepSessionError::ok){next->~Data();free(next);return error;}
 next->rules_crc=crc(rules,rn);out=next;return UpkeepSessionError::ok;
}
UpkeepSessionError FacilityUpkeepSession::start(const uint8_t* save,size_t n,const uint8_t* rules,size_t rn,uint32_t seed,size_t limit){
 if(data_)return UpkeepSessionError::bad_state;
 Data* next=nullptr;const auto e=prepare(save,n,rules,rn,seed,limit,next);
 if(e==UpkeepSessionError::ok)data_=next;
 return e;
}
UpkeepSessionError FacilityUpkeepSession::resume(const uint8_t* input,size_t n,const uint8_t* rules,size_t rn,size_t limit){
 if(!input || n<header_size || memcmp(input,magic,8) || get(input+8)!=header_size ||
    get(input+12)!=n-header_size || get(input+28)!=checkpoint_crc(input,n) ||
    get(input+24)!=crc(input+header_size,n-header_size) || get(input+40)!=1)return UpkeepSessionError::bad_checkpoint;
 for(size_t i=44;i<64;++i)if(input[i])return UpkeepSessionError::bad_checkpoint;
 if(!rules || !rn || get(input+20)!=crc(rules,rn))return UpkeepSessionError::bad_rules;
 Data* next=nullptr;const auto error=prepare(input+header_size,n-header_size,rules,rn,get(input+16),limit,next);
 if(error!=UpkeepSessionError::ok)return error;
 bool valid=get(input+32)==next->world.game().base_count && get(input+36)==(uint32_t)next->world.game().player;
 for(size_t i=0;i<512 && valid;++i){
  const bool p=bit(input+64,i),r=bit(input+128,i),c=bit(input+192,i);
  if(r!=c || (r && !p)){valid=false;break;}
  if(!p)continue;
  const auto* base=next->world.base(i);
  if(!base || base->faction_id!=next->world.game().player){valid=false;break;}
  if(r){const int32_t item=base->queue_items[0];
   if((item!=-2 && item!=-3) || !((uint32_t)base->state_flags&0x00800000u) ||
      !(base->facilities_built[0]&(1u<<(unsigned)-item)))valid=false;
  }
 }
 if(!valid){next->~Data();free(next);return UpkeepSessionError::bad_checkpoint;}
 memcpy(next->processed,input+64,64);memcpy(next->recompute,input+128,64);memcpy(next->choice,input+192,64);
 Data* prior=data_;data_=next;if(prior){prior->~Data();free(prior);}return UpkeepSessionError::ok;
}
FacilityProductionError FacilityUpkeepSession::apply_fresh_materials(size_t id,const StableIntakeStage& stage,FacilityStageReceipt& output){
 if(!data_)return FacilityProductionError::bad_state;
 if(overlap(&output,sizeof(output),this,sizeof(*this)) || overlap(&stage,sizeof(stage),this,sizeof(*this)) ||
    overlap(&output,sizeof(output),data_,sizeof(Data)) || overlap(&stage,sizeof(stage),data_,sizeof(Data)) ||
    aliases(data_->world,&output,sizeof(output)) || aliases(data_->world,&stage,sizeof(stage)) ||
    overlap(&output,sizeof(output),&stage,sizeof(stage)))return FacilityProductionError::bad_buffer;
 if(id>=data_->world.game().base_count || id>=512)return FacilityProductionError::bad_state;
 if(bit(data_->processed,id))return FacilityProductionError::already_processed;
 FacilityProductionStage transient(data_->world,data_->technologies,data_->basic,data_->facilities);
 FacilityStageReceipt receipt{};
 const auto error=transient.apply_fresh_materials(id,data_->units,data_->world_rules,data_->resources,stage,receipt);
 if(error!=FacilityProductionError::ok)return error;
 set(data_->processed,id);
 if(receipt.recompute_required)set(data_->recompute,id);
 if(receipt.production_choice_required)set(data_->choice,id);
 output=receipt;return FacilityProductionError::ok;
}
size_t FacilityUpkeepSession::checkpoint_size()const{
 return data_ && data_->world.original_size()<=UINT32_MAX && data_->world.original_size()<=SIZE_MAX-header_size?header_size+data_->world.original_size():0;
}
LocalSnapshotError FacilityUpkeepSession::checkpoint(void* output,size_t capacity)const{
 if(!data_)return LocalSnapshotError::bad_world;
 const size_t n=checkpoint_size();
 if(!n || !output || capacity<n || overlap(output,n,this,sizeof(*this)) || overlap(output,n,data_,sizeof(Data)) || aliases(data_->world,output,n))return LocalSnapshotError::bad_buffer;
 auto* temporary=(uint8_t*)calloc(1,n);if(!temporary)return LocalSnapshotError::out_of_memory;
 LocalSnapshotReceipt receipt{};
 const auto error=encode_facility_production_snapshot(data_->world,temporary+header_size,n-header_size,receipt);
 if(error==LocalSnapshotError::ok){
  memcpy(temporary,magic,8);put(temporary+8,header_size);put(temporary+12,(uint32_t)(n-header_size));
  put(temporary+16,receipt.runtime_seed);put(temporary+20,data_->rules_crc);put(temporary+24,crc(temporary+header_size,n-header_size));
  put(temporary+32,(uint32_t)data_->world.game().base_count);put(temporary+36,(uint32_t)data_->world.game().player);put(temporary+40,1);
  memcpy(temporary+64,data_->processed,64);memcpy(temporary+128,data_->recompute,64);memcpy(temporary+192,data_->choice,64);
  put(temporary+28,checkpoint_crc(temporary,n));memcpy(output,temporary,n);
 }
 free(temporary);return error;
}
}
