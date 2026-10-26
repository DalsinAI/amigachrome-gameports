/* SPDX-License-Identifier: GPL-3.0-or-later. See unreferenced_prototype_allocation.h. */
#include "unreferenced_prototype_allocation.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {
void UnreferencedAllocationPlan::swap(UnreferencedAllocationPlan& other){allocation_.swap(other.allocation_);auto h=history_;history_=other.history_;other.history_=h;bool b=ready_;ready_=other.ready_;other.ready_=b;b=retired_;retired_=other.retired_;other.retired_=b;}
static bool alias(const OriginalWorld& w,const void* a,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(a,n,&w,sizeof w) || w.overlaps_byte_storage(a,n) || effects_array_overlap(a,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(a,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(a,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(a,n,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(a,n,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(a,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
static UnreferencedAllocationError mapped(InactiveAllocationError e){switch(e){case InactiveAllocationError::ok:return UnreferencedAllocationError::ok;case InactiveAllocationError::runtime_required:return UnreferencedAllocationError::runtime_required;case InactiveAllocationError::existing_return_required:return UnreferencedAllocationError::existing_return_required;case InactiveAllocationError::strength_required:return UnreferencedAllocationError::strength_required;case InactiveAllocationError::no_slot:return UnreferencedAllocationError::no_slot;case InactiveAllocationError::live_reference_required:return UnreferencedAllocationError::live_reference_required;case InactiveAllocationError::naming_context_required:return UnreferencedAllocationError::naming_context_required;case InactiveAllocationError::naming_nonreturning:return UnreferencedAllocationError::naming_nonreturning;case InactiveAllocationError::overflow:return UnreferencedAllocationError::overflow;case InactiveAllocationError::out_of_memory:return UnreferencedAllocationError::out_of_memory;default:return UnreferencedAllocationError::bad_state;}}
UnreferencedAllocationError world_unreferenced_prototype_allocation(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const ExistingProposalRequest& q,const InactiveAllocationFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,UnreferencedAllocationPlan& out){using detail::effects_range_overlap;
 auto overlaps=[&](const void* a,size_t n){return alias(w,a,n) || effects_range_overlap(a,n,&r,sizeof r) || effects_range_overlap(a,n,&d,sizeof d) || effects_range_overlap(a,n,&q,sizeof q) || effects_range_overlap(a,n,&fact,sizeof fact) || effects_range_overlap(a,n,&labels,sizeof labels) || effects_range_overlap(a,n,&suffix,sizeof suffix);};
 if(overlaps(&out,sizeof out) || (out.allocation_.count_ && (out.allocation_.count_>512 || overlaps(out.allocation_.bases_,out.allocation_.count_*sizeof(OriginalBase)))) || !w.loaded())return UnreferencedAllocationError::bad_state;
 InactiveAllocationPlan result;const auto initial=world_inactive_prototype_allocation(w,v,r,d,q,fact,labels,suffix,result);
 if(initial!=InactiveAllocationError::ok && initial!=InactiveAllocationError::retirement_required)return mapped(initial);
 const auto table=w.game().bunglist;if(!table.data || table.size!=4096)return UnreferencedAllocationError::bad_state;
 RetirementHistoryResult history{};bool retired=false;
 if(initial==InactiveAllocationError::ok){memcpy(history.table,table.data,4096);const auto* f=w.faction((size_t)q.faction);if(!f)return UnreferencedAllocationError::bad_state;history.next_slot=f->unk_103;}
 else{
 const auto* state=w.faction((size_t)q.faction);if(!state)return UnreferencedAllocationError::bad_state;
 const uint8_t bit=(uint8_t)(1u<<(unsigned)q.faction);int32_t chosen=-1,best=0;
 for(int32_t id=q.faction*64;id<(q.faction+1)*64;++id){const auto* p=w.prototype((size_t)id);if(!p)return UnreferencedAllocationError::bad_state;const uint32_t active=state->units_active[id],queued=state->units_queue[id];int32_t score=0;
  if((p->unit_flags&1u) && (p->obsolete_factions&bit) && !active && !queued)score=((uint32_t)(uint8_t)p->group_id==(uint32_t)q.plan)?0x2000:0x1000;
  else if(!(p->unit_flags&1u))score=0x800;
  else if((p->obsolete_factions&bit) && !active)score=0x400;
  else if((uint32_t)(uint8_t)p->group_id==(uint32_t)q.plan || (!active && !queued)){uint32_t value=active?0x40u:0x240u;if(p->obsolete_factions&bit)value|=0x100u;if(!queued)value|=0x80u;uint32_t cost=(uint32_t)p->cost*active+queued;if(cost>63)cost=63;score=(int32_t)(value-cost);}
  if(score>best){chosen=id;best=score;}
 }

 if(chosen<0)return UnreferencedAllocationError::no_slot;
 const auto& prior=*w.prototype((size_t)chosen);if(!(prior.unit_flags&1u))return UnreferencedAllocationError::bad_state;
 UnusedRetirementProposal retirement;const auto re=world_unused_prototype_retirement(w,v,q.faction,chosen,retirement);if(re!=UnusedRetirementError::ok){if(re==UnusedRetirementError::live_unit_effects_required)return UnreferencedAllocationError::live_reference_required;if(re==UnusedRetirementError::out_of_memory)return UnreferencedAllocationError::out_of_memory;return UnreferencedAllocationError::bad_state;}
 for(size_t i=0;i<retirement.queues.count();++i){const auto* b=retirement.queues.base(i);for(int32_t j=0;j<=b->queue_size;++j)if(b->queue_items[j]>=512)return UnreferencedAllocationError::bad_state;}
 auto post=prior;post.unit_flags=retirement.queues.unit_flags();CustomPrototypeRequest request{chosen,q.chassis,q.weapon,q.armor,q.reactor,q.abilities};OriginalPrototype constructed{},prepared{},stored{};
 const auto le=world_construct_custom_prototype_after_retirement(w,v,r,d,request,post,constructed);if(le!=LogicError::ok)return le==LogicError::overflow?UnreferencedAllocationError::overflow:UnreferencedAllocationError::bad_state;
 const ProposerPreparationFacts pf{q.plan,fact.difficulty};if(prepare_proposer_record(prior,constructed,pf,prepared)!=ProposerRecordError::ok)return UnreferencedAllocationError::bad_state;
 PrototypeLogicalName name{};const auto ne=world_resolve_automatic_prototype_logical_name_history(w,v,r,d,labels,suffix,chosen,prepared,retirement.history,name);if(ne!=PrototypeNameError::ok){if(ne==PrototypeNameError::original_would_not_return)return UnreferencedAllocationError::naming_nonreturning;if(ne==PrototypeNameError::collision_context_required)return UnreferencedAllocationError::naming_context_required;if(ne==PrototypeNameError::overflow)return UnreferencedAllocationError::overflow;return UnreferencedAllocationError::bad_state;}
 if(store_proposer_automatic_name(prepared,name,stored)!=ProposerRecordError::ok)return UnreferencedAllocationError::bad_state;
 HumanPruningResult prune{};if(world_human_component_pruning_candidate(w,v,r,d,fact.multiplayer,q.faction,chosen,stored,prune)!=HumanPruningError::ok)return UnreferencedAllocationError::bad_state;
 result.id_=chosen;result.last_=fact.last_created<0?chosen:fact.last_created;result.selected_=stored;result.count_=retirement.queues.count();for(unsigned i=0;i<512;++i)result.flags_[i]=prune.flags[i];result.selected_.unit_flags=prune.flags[chosen];
 if(result.count_){result.bases_=(OriginalBase*)malloc(result.count_*sizeof(OriginalBase));if(!result.bases_)return UnreferencedAllocationError::out_of_memory;for(size_t i=0;i<result.count_;++i)result.bases_[i]=*retirement.queues.base(i);}
 for(size_t i=0;i<result.count_;++i){auto& b=result.bases_[i];if(b.faction_id!=q.faction)continue;for(int32_t j=0;j<=b.queue_size;++j){const int32_t id=b.queue_items[j];if(id<0)continue;const auto& p=id==chosen?result.selected_:*w.prototype((size_t)id);if((p.obsolete_factions&bit) && p.group_id==result.selected_.group_id)b.queue_items[j]=chosen;}}
 result.ready_=true;history=retirement.history;retired=true;
 }
 out.allocation_.swap(result);out.history_=history;out.retired_=retired;out.ready_=true;return UnreferencedAllocationError::ok;
}
}
