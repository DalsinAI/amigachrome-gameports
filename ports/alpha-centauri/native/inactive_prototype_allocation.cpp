/* SPDX-License-Identifier: GPL-3.0-or-later. See inactive_prototype_allocation.h. */
#include "inactive_prototype_allocation.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
#include <utility>
namespace alpha {
void InactiveAllocationPlan::clear(){free(bases_);bases_=nullptr;count_=0;selected_={};memset(flags_,0,sizeof flags_);id_=last_=-1;ready_=false;}
void InactiveAllocationPlan::swap(InactiveAllocationPlan& p){using std::swap;swap(selected_,p.selected_);for(unsigned i=0;i<512;++i)swap(flags_[i],p.flags_[i]);swap(bases_,p.bases_);swap(count_,p.count_);swap(id_,p.id_);swap(last_,p.last_);swap(ready_,p.ready_);}
static bool alias(const OriginalWorld& w,const void* a,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(a,n,&w,sizeof w) || w.overlaps_byte_storage(a,n) || effects_array_overlap(a,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(a,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(a,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(a,n,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(a,n,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(a,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
InactiveAllocationError world_inactive_prototype_allocation(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,const ExistingProposalRequest& q,const InactiveAllocationFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,InactiveAllocationPlan& out){using detail::effects_range_overlap;
 auto overlaps=[&](const void* a,size_t n){return alias(w,a,n) || effects_range_overlap(a,n,&r,sizeof r) || effects_range_overlap(a,n,&d,sizeof d) || effects_range_overlap(a,n,&q,sizeof q) || effects_range_overlap(a,n,&fact,sizeof fact) || effects_range_overlap(a,n,&labels,sizeof labels) || effects_range_overlap(a,n,&suffix,sizeof suffix);};
 if(overlaps(&out,sizeof out) || (out.count_ && (out.count_>512 || overlaps(out.bases_,out.count_*sizeof(OriginalBase)))) || !w.loaded() || w.game().base_count>512)return InactiveAllocationError::bad_state;
 ExistingProposalResult early{};const auto e=world_existing_prototype_proposal(w,v,r,d,q,early);
 if(e!=ExistingProposalError::allocation_required){if(e==ExistingProposalError::ok)return InactiveAllocationError::existing_return_required;if(e==ExistingProposalError::unsupported_runtime)return InactiveAllocationError::runtime_required;if(e==ExistingProposalError::unsupported_strength)return InactiveAllocationError::strength_required;return InactiveAllocationError::bad_state;}
 if(fact.multiplayer!=PruningRuntime::multiplayer_off || fact.cursor!=AllocationCursorObservation::supplied || (w.game().preferences.GameMorePreferences&0x100000))return InactiveAllocationError::runtime_required;
 if(v!=Variant::base)return InactiveAllocationError::naming_context_required;
 const auto* state=w.faction((size_t)q.faction);if(!state)return InactiveAllocationError::bad_state;
 const uint8_t bit=(uint8_t)(1u<<(unsigned)q.faction);int32_t chosen=-1,best=0;
 for(int32_t id=q.faction*64;id<(q.faction+1)*64;++id){const auto* p=w.prototype((size_t)id);if(!p)return InactiveAllocationError::bad_state;const uint32_t active=state->units_active[id],queued=state->units_queue[id];int32_t score=0;
  if((p->unit_flags&1u) && (p->obsolete_factions&bit) && !active && !queued)score=((uint32_t)(uint8_t)p->group_id==(uint32_t)q.plan)?0x2000:0x1000;
  else if(!(p->unit_flags&1u))score=0x800;
  else if((p->obsolete_factions&bit) && !active)score=0x400;
  else if((uint32_t)(uint8_t)p->group_id==(uint32_t)q.plan || (!active && !queued)){uint32_t value=active?0x40u:0x240u;if(p->obsolete_factions&bit)value|=0x100u;if(!queued)value|=0x80u;uint32_t cost=(uint32_t)p->cost*active+queued;if(cost>63)cost=63;score=(int32_t)(value-cost);}
  if(score>best){chosen=id;best=score;}
 }
 if(chosen<0)return InactiveAllocationError::no_slot;
 const auto& prior=*w.prototype((size_t)chosen);if(prior.unit_flags&1u)return InactiveAllocationError::retirement_required;
 for(size_t i=0;i<w.game().vehicle_count;++i)if(w.vehicle(i)->unit_id==chosen)return InactiveAllocationError::live_reference_required;
 for(size_t i=0;i<w.game().base_count;++i){const auto* b=w.base(i);if(!b || b->queue_size < -1 || b->queue_size>9)return InactiveAllocationError::bad_state;for(int32_t j=0;j<=b->queue_size;++j)if(b->queue_items[j]>=512)return InactiveAllocationError::bad_state;}
 CustomPrototypeRequest request{chosen,q.chassis,q.weapon,q.armor,q.reactor,q.abilities};OriginalPrototype constructed{},prepared{},stored{};
 auto le=world_construct_custom_prototype(w,v,r,d,request,constructed);if(le!=LogicError::ok)return le==LogicError::overflow?InactiveAllocationError::overflow:InactiveAllocationError::bad_state;
 const ProposerPreparationFacts pf{q.plan,fact.difficulty};if(prepare_proposer_record(prior,constructed,pf,prepared)!=ProposerRecordError::ok)return InactiveAllocationError::bad_state;
 PrototypeLogicalName name{};const auto ne=world_resolve_automatic_prototype_logical_name(w,v,r,d,labels,suffix,chosen,prepared,name);if(ne!=PrototypeNameError::ok){if(ne==PrototypeNameError::original_would_not_return)return InactiveAllocationError::naming_nonreturning;if(ne==PrototypeNameError::collision_context_required)return InactiveAllocationError::naming_context_required;if(ne==PrototypeNameError::overflow)return InactiveAllocationError::overflow;return InactiveAllocationError::bad_state;}
 if(store_proposer_automatic_name(prepared,name,stored)!=ProposerRecordError::ok)return InactiveAllocationError::bad_state;
 HumanPruningResult prune{};if(world_human_component_pruning_candidate(w,v,r,d,fact.multiplayer,q.faction,chosen,stored,prune)!=HumanPruningError::ok)return InactiveAllocationError::bad_state;
 InactiveAllocationPlan next;next.id_=chosen;next.last_=fact.last_created<0?chosen:fact.last_created;next.selected_=stored;next.count_=w.game().base_count;for(unsigned i=0;i<512;++i)next.flags_[i]=prune.flags[i];next.selected_.unit_flags=prune.flags[chosen];
 if(next.count_){next.bases_=(OriginalBase*)malloc(next.count_*sizeof(OriginalBase));if(!next.bases_)return InactiveAllocationError::out_of_memory;memcpy(next.bases_,w.base(0),next.count_*sizeof(OriginalBase));}
 for(size_t i=0;i<next.count_;++i){auto& b=next.bases_[i];if(b.faction_id!=q.faction)continue;for(int32_t j=0;j<=b.queue_size;++j){const int32_t id=b.queue_items[j];if(id<0)continue;const auto& p=id==chosen?next.selected_:*w.prototype((size_t)id);if((p.obsolete_factions&bit) && p.group_id==next.selected_.group_id)b.queue_items[j]=chosen;}}
 next.ready_=true;out.swap(next);return InactiveAllocationError::ok;
}
}
