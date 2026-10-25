/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "local_production_session.h"
#include <stdlib.h>
#include <new>
#include <limits.h>
namespace alpha {
struct LocalProductionSession::Data {
 OriginalWorld world;
 Rules technology;
 FacilityDefinitions facilities;
 BasicRules rules;
 WorldRules world_rules;
 Variant profile;
 bool expansion_enabled;
 int32_t local_human;
};
namespace {
uint64_t identity_sequence=0;
bool at_limit(uint64_t revision){return revision==UINT64_MAX;}
bool item_bounded(int32_t item,Variant profile){return item>=0?item<512:item>-(profile==Variant::base?72:134);}
LocalOrderError translate(LogicError e){return e==LogicError::overflow?LocalOrderError::overflow:LocalOrderError::bad_state;}
}
LocalProductionSession::LocalProductionSession(){if(!at_limit(identity_sequence))identity_=++identity_sequence;}
void LocalProductionSession::close(){
 if(data_){data_->~Data();free(data_);data_=nullptr;if(!at_limit(revision_))++revision_;}
 suspended_=false;
}
const OriginalWorld* LocalProductionSession::world()const{return data_?&data_->world:nullptr;}
int32_t LocalProductionSession::local_human()const{return data_?data_->local_human:-1;}
LocalOrderError LocalProductionSession::load_local(const uint8_t* save,size_t length,const uint8_t* text,size_t text_length,
 Variant profile,bool expansion,int32_t human,uint32_t seed,size_t memory_limit){
 if(!save || !text || !length || !text_length || (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    (profile==Variant::base && expansion) || human<1 || human>7)return LocalOrderError::bad_state;
 if(!identity_ || at_limit(revision_))return LocalOrderError::overflow;
 if(memory_limit<sizeof(Data))return LocalOrderError::no_memory;
 void* storage=malloc(sizeof(Data));if(!storage)return LocalOrderError::no_memory;
 Data* next=new(storage)Data;LocalOrderError result=LocalOrderError::ok;
 if(read_technologies(text,text_length,next->technology)!=Error::ok ||
    read_basic_rules(text,text_length,next->technology,profile,next->rules,next->world_rules)!=Error::ok ||
    read_facility_definitions(text,text_length,next->technology,profile,next->facilities)!=Error::ok)result=LocalOrderError::bad_rules;
 // The original file loader clamps strictness to2. The standalone retool API
 // also supports3, but does not imply that original text loading accepted3.
 if(result==LocalOrderError::ok && next->rules.retool_strictness>2)next->rules.retool_strictness=2;
 if(result==LocalOrderError::ok){
  const auto loaded=next->world.load(save,length,seed,memory_limit-sizeof(Data)+sizeof(OriginalWorld));
  if(loaded!=WorldLoadError::ok)result=(loaded==WorldLoadError::memory_limit || loaded==WorldLoadError::out_of_memory)?LocalOrderError::no_memory:LocalOrderError::bad_world;
 }
 if(result==LocalOrderError::ok){
  const auto& game=next->world.game();
  if((game.data_version<11)!=(profile==Variant::base))result=LocalOrderError::unsupported;
  else if(game.player!=human || !(game.preferences.FactionStatus[0]&(1u<<(unsigned)human)))result=LocalOrderError::not_human;
  else if(game.preferences.CurrentFaction!=human)result=LocalOrderError::unsupported; // Saved active turn faction, not MapWin owner.
  else {WorldFacilityOptions options;const auto e=world_facility_options(next->world,next->technology,next->facilities,profile,expansion,human,-1,0,options);if(e!=LogicError::ok)result=translate(e);}
 }
 if(result!=LocalOrderError::ok){next->~Data();free(next);return result;}
 next->profile=profile;next->expansion_enabled=expansion;next->local_human=human;
 Data* previous=data_;data_=next;++revision_;suspended_=false;
 if(previous){previous->~Data();free(previous);}
 return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::set_suspended(bool value){
 if(!data_)return LocalOrderError::bad_state;
 if(value==suspended_)return LocalOrderError::ok;
 if(at_limit(revision_))return LocalOrderError::overflow;
 suspended_=value;++revision_;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::prepare_facility(int32_t base_id,int32_t facility,FacilityOrderPlan& output)const{
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(base_id<0 || facility<1 || (size_t)facility>=data_->facilities.slot_count)return LocalOrderError::bad_state;
 const auto* base=data_->world.base((size_t)base_id);if(!base)return LocalOrderError::bad_state;
 if(base->faction_id!=data_->local_human)return LocalOrderError::not_owner;
 if((uint32_t)base->state_flags&0x10000000u)return LocalOrderError::unsupported; // Original BSTATE_NET_LOCKED.
 // Queue edits with multiple/empty slots require their own source-backed
 // contract. A first-item replacement does not fabricate queue compaction.
 if(base->queue_size!=0)return LocalOrderError::unsupported;
 if(!item_bounded(base->queue_items[0],data_->profile) || !item_bounded(base->production_id_last,data_->profile))return LocalOrderError::bad_state;
 FacilityOrderPlan next;next.owner_identity_=identity_;next.revision_=revision_;
 auto& p=next.preview_;p.base=base_id;p.facility=facility;p.previous_item=base->queue_items[0];p.proposed_item=-facility;
 p.mineral_before=base->minerals_accumulated;p.mineral_baseline=base->minerals_accumulated_2;p.mineral_after=p.mineral_before;p.changes_state=p.previous_item!=p.proposed_item;
 // The real original same-item path returns before mineral/queue-counter work.
 // It remains an idempotent no-op even if an existing target later becomes
 // unavailable; it grants no fresh production capability.
 if(p.changes_state){
  WorldFacilityOptions options;auto error=world_facility_options(data_->world,data_->technology,data_->facilities,data_->profile,data_->expansion_enabled,data_->local_human,base_id,0,options);
  if(error!=LogicError::ok)return translate(error);
  if(!options.available[facility])return LocalOrderError::unavailable;
  ProductionRetoolResult retool;error=world_production_retool(data_->world,data_->rules,data_->profile,(size_t)base_id,p.proposed_item,retool);
  if(error!=LogicError::ok)return translate(error);
  const int64_t after=(int64_t)p.mineral_baseline-retool.mineral_loss;if(after<INT32_MIN || after>INT32_MAX)return LocalOrderError::overflow;
  p.mineral_loss=retool.mineral_loss;p.mineral_after=(int32_t)after;
 }
 output=next;return LocalOrderError::ok;
}
LocalOrderError LocalProductionSession::commit(const FacilityOrderPlan& plan,FacilityOrderReceipt& output){
 if(!data_)return LocalOrderError::bad_state;
 if(suspended_)return LocalOrderError::suspended;
 if(!plan.owner_identity_ || plan.owner_identity_!=identity_ || plan.revision_!=revision_)return LocalOrderError::stale_plan;
 FacilityOrderPlan verified;auto error=prepare_facility(plan.preview_.base,plan.preview_.facility,verified);if(error!=LocalOrderError::ok)return error;
 const auto& a=plan.preview_;const auto& b=verified.preview_;
 if(a.previous_item!=b.previous_item || a.proposed_item!=b.proposed_item || a.mineral_before!=b.mineral_before ||
    a.mineral_baseline!=b.mineral_baseline || a.mineral_loss!=b.mineral_loss || a.mineral_after!=b.mineral_after ||
    a.changes_state!=b.changes_state)return LocalOrderError::stale_plan;
 if(b.changes_state && at_limit(revision_))return LocalOrderError::overflow;
 FacilityOrderReceipt next{b.changes_state,b.changes_state,b.changes_state,revision_,b};
 if(b.changes_state){
  // All bounds, eligibility, arithmetic and confirmation checks precede writes.
  // This class owns the world; no mutable world/rules handle is exposed.
  auto& base=data_->world.bases_[(size_t)b.base];auto& faction=data_->world.factions_[(size_t)data_->local_human];
  if(b.previous_item>=0){auto& count=faction.units_queue[(size_t)b.previous_item];count=(uint8_t)(count-1u);}
  base.queue_items[0]=b.proposed_item;base.minerals_accumulated=b.mineral_after;
  ++revision_;next.revision=revision_;
 }
 output=next;return LocalOrderError::ok;
}
const char* local_order_error_name(LocalOrderError e){switch(e){
 case LocalOrderError::ok:return "ok";case LocalOrderError::bad_state:return "bad_state";case LocalOrderError::bad_rules:return "bad_rules";
 case LocalOrderError::bad_world:return "bad_world";case LocalOrderError::no_memory:return "no_memory";case LocalOrderError::not_human:return "not_human";
 case LocalOrderError::not_owner:return "not_owner";case LocalOrderError::unavailable:return "unavailable";case LocalOrderError::unsupported:return "unsupported";
 case LocalOrderError::stale_plan:return "stale_plan";case LocalOrderError::suspended:return "suspended";case LocalOrderError::overflow:return "overflow";
 }return "unknown";}
}
