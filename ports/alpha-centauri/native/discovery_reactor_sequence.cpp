/* SPDX-License-Identifier: GPL-3.0-or-later. See discovery_reactor_sequence.h. */
#include "discovery_reactor_sequence.h"
#include "technology_effects_ranges.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include <type_traits>
namespace alpha {
namespace {
template<class T>T* copied(const T* from,size_t count){
 static_assert(std::is_trivially_copyable<T>::value,"snapshot records must copy safely");
 if(!count)return nullptr;
 if(!from || count>SIZE_MAX/sizeof(T))return nullptr;
 auto* p=(T*)malloc(count*sizeof(T));if(p){for(size_t i=0;i<count;++i)new(p+i)T;memcpy(p,from,count*sizeof(T));}return p;
}
bool alias(const OriginalWorld& w,const void* a,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(a,n,&w,sizeof w) || w.overlaps_byte_storage(a,n) || effects_array_overlap(a,n,w.game().bunglist.data,w.game().bunglist.size,1) || effects_array_overlap(a,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(a,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(a,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(a,n,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(a,n,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(a,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
DiscoveryReactorError mapped(UnreferencedAllocationError e){switch(e){case UnreferencedAllocationError::ok:return DiscoveryReactorError::ok;case UnreferencedAllocationError::runtime_required:return DiscoveryReactorError::runtime_required;case UnreferencedAllocationError::existing_return_required:case UnreferencedAllocationError::no_slot:case UnreferencedAllocationError::strength_required:return DiscoveryReactorError::proposal_required;case UnreferencedAllocationError::live_reference_required:return DiscoveryReactorError::live_reference_required;case UnreferencedAllocationError::naming_context_required:return DiscoveryReactorError::naming_context_required;case UnreferencedAllocationError::naming_nonreturning:return DiscoveryReactorError::naming_nonreturning;case UnreferencedAllocationError::overflow:return DiscoveryReactorError::overflow;case UnreferencedAllocationError::out_of_memory:return DiscoveryReactorError::out_of_memory;default:return DiscoveryReactorError::bad_state;}}
}
DiscoveryReactorError DiscoveryReactorSequencePlan::capture(const OriginalWorld& w,size_t limit){
 if(!w.loaded() || !w.game().bunglist.data || w.game().bunglist.size!=4096 || w.game().base_count>512)return DiscoveryReactorError::bad_state;
 const auto& g=w.game();const uint64_t arrays=(uint64_t)g.map.info.tile_count*sizeof(OriginalMapTile)+(uint64_t)g.vehicle_count*sizeof(OriginalVehicle)+(uint64_t)g.base_count*sizeof(OriginalBase)+512u*(uint64_t)sizeof(OriginalPrototype)+8u*(uint64_t)(sizeof(OriginalFactionState)+sizeof(OriginalFactionDefinition));
 const uint64_t required=sizeof(*this)+(uint64_t)w.length_+0x27a0u+arrays;if(required>limit || required>SIZE_MAX)return DiscoveryReactorError::out_of_memory;
 frame_.length_=w.length_;frame_.memory_=(size_t)required-sizeof(*this)+sizeof(OriginalWorld);frame_.random_seed_=w.random_seed_;frame_.view_=g;
 frame_.raw_=copied(w.raw_,w.length_);frame_.monuments_=copied(w.monuments_,size_t(0x27a0));
 frame_.tiles_=copied(w.tiles_,g.map.info.tile_count);frame_.vehicles_=copied(w.vehicles_,g.vehicle_count);frame_.bases_=copied(w.bases_,g.base_count);frame_.prototypes_=copied(w.prototypes_,512);frame_.factions_=copied(w.factions_,8);frame_.definitions_=copied(w.definitions_,8);
 if(!frame_.raw_ || !frame_.monuments_ || !frame_.tiles_ || !frame_.prototypes_ || !frame_.factions_ || !frame_.definitions_ || (g.vehicle_count && !frame_.vehicles_) || (g.base_count && !frame_.bases_))return DiscoveryReactorError::out_of_memory;
 auto relocate=[&](const uint8_t* source,size_t size,const uint8_t*& destination){if(!source && !size){destination=nullptr;return true;}const uintptr_t x=(uintptr_t)source,start=(uintptr_t)w.raw_;if(x<start || x-start>w.length_ || size>w.length_-(size_t)(x-start))return false;destination=frame_.raw_+(size_t)(x-start);return true;};
 auto span=[&](OriginalByteSpan& s){return relocate(s.data,s.size,s.data);};
 if(!relocate(g.map.metadata,original_map_metadata_size,frame_.view_.map.metadata) || !relocate(g.map.tiles,(size_t)g.map.info.tile_count*original_map_tile_size,frame_.view_.map.tiles) || !relocate(g.map.abstract,g.map.info.abstract_count,frame_.view_.map.abstract))return DiscoveryReactorError::bad_state;
#define REBASE(name) if(!span(frame_.view_.name))return DiscoveryReactorError::bad_state;
 REBASE(factions) REBASE(faction_definitions) REBASE(rankings) REBASE(faction_turn_might) REBASE(replay) REBASE(continents) REBASE(prototypes) REBASE(vehicles) REBASE(bases) REBASE(scenario) REBASE(scenario_name) REBASE(objectives) REBASE(monuments) REBASE(tectonic)
#undef REBASE
 memcpy(history_,g.bunglist.data,4096);frame_.view_.bunglist={history_,4096};return DiscoveryReactorError::ok;
}
void DiscoveryReactorSequencePlan::apply_existing(const ExistingProposalResult& p){if(p.kind==ExistingProposalKind::reused)frame_.prototypes_[p.id]=p.prototype;}
void DiscoveryReactorSequencePlan::apply_flags(const HumanPruningResult& p){for(unsigned i=0;i<512;++i)frame_.prototypes_[i].unit_flags=p.flags[i];}
DiscoveryReactorError DiscoveryReactorSequencePlan::apply_allocation(const UnreferencedAllocationPlan& p,int32_t faction){
 const auto& a=p.allocation();if(!p.ready() || !a.ready() || a.id()<64 || a.id()>=512 || a.count()!=frame_.game().base_count)return DiscoveryReactorError::bad_state;
 frame_.prototypes_[a.id()]=a.selected();for(unsigned i=0;i<512;++i)frame_.prototypes_[i].unit_flags=a.flags(i);
 for(size_t i=0;i<a.count();++i){const auto* b=a.base(i);if(!b)return DiscoveryReactorError::bad_state;frame_.bases_[i]=*b;}
 memcpy(history_,p.history().table,4096);frame_.factions_[faction].unk_103=p.history().next_slot;return DiscoveryReactorError::ok;
}
void DiscoveryReactorSequencePlan::swap(DiscoveryReactorSequencePlan& p){frame_.swap(p.frame_);for(unsigned i=0;i<4096;++i){const auto n=history_[i];history_[i]=p.history_[i];p.history_[i]=n;}for(unsigned i=0;i<192;++i){auto e=events_[i];events_[i]=p.events_[i];p.events_[i]=e;}auto n=count_;count_=p.count_;p.count_=n;auto u=upgrade_;upgrade_=p.upgrade_;p.upgrade_=u;auto exchange=[](auto& a,auto& b){auto t=a;a=b;b=t;};exchange(last_,p.last_);exchange(cache_a_,p.cache_a_);exchange(cache_b_,p.cache_b_);exchange(reactor_,p.reactor_);exchange(boundary_,p.boundary_);for(unsigned i=0;i<128;++i)exchange(reactor_name_[i],p.reactor_name_[i]);bool b=ready_;ready_=p.ready_;p.ready_=b;if(frame_.loaded())frame_.view_.bunglist={history_,4096};if(p.frame_.loaded())p.frame_.view_.bunglist={p.history_,4096};}
DiscoveryReactorError discovery_reactor_sequence_impl(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,int32_t f,int32_t tech,const DiscoveryReactorFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,const DiscoveryReactorBoundaryFacts* boundaryFacts,DiscoveryReactorSequencePlan& out,size_t limit){using detail::effects_range_overlap;
 auto overlap=[&](const void* p,size_t n){return alias(w,p,n) || effects_range_overlap(p,n,&r,sizeof r) || effects_range_overlap(p,n,&d,sizeof d) || effects_range_overlap(p,n,&fact,sizeof fact) || effects_range_overlap(p,n,&labels,sizeof labels) || effects_range_overlap(p,n,&suffix,sizeof suffix) || (boundaryFacts && effects_range_overlap(p,n,boundaryFacts,sizeof *boundaryFacts));};
 if(overlap(&out,sizeof out) || !w.loaded() || f<1 || f>7 || tech<0 || tech>=89 || (v!=Variant::base && v!=Variant::alien_crossfire) || ((w.game().data_version<11)!=(v==Variant::base)) || r.count!=89)return DiscoveryReactorError::bad_state;
 const auto& old=out.frame_;if(old.loaded() && (overlap(old.original_bytes(),old.original_size()) || overlap(old.current_monuments().data,old.current_monuments().size) || overlap(old.tile(0),old.game().map.info.tile_count*sizeof(OriginalMapTile)) || overlap(old.vehicle(0),old.game().vehicle_count*sizeof(OriginalVehicle)) || overlap(old.base(0),old.game().base_count*sizeof(OriginalBase)) || overlap(old.prototype(0),512*sizeof(OriginalPrototype)) || overlap(old.faction(0),8*sizeof(OriginalFactionState)) || overlap(old.faction_definition(0),8*sizeof(OriginalFactionDefinition))))return DiscoveryReactorError::bad_state;
 if(boundaryFacts && boundaryFacts->runtime!=DiscoveryRuntimeObservation::supplied)return DiscoveryReactorError::runtime_required;
 if(v!=Variant::base)return DiscoveryReactorError::naming_context_required;
 if(fact.runtime!=DiscoveryRuntimeObservation::supplied || (fact.skip!=SkipTechnologyScreenObservation::zero && fact.skip!=SkipTechnologyScreenObservation::nonzero) || fact.map_owner<0 || fact.map_owner>7 || fact.proposal_mode!=ProposalMode::normal || fact.allocation.multiplayer!=PruningRuntime::multiplayer_off || fact.allocation.cursor!=AllocationCursorObservation::supplied || !(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)f)) || (w.game().preferences.GameMorePreferences&0x100000) || (w.game().preferences.GamePreferences&0x4000))return DiscoveryReactorError::runtime_required;
 if(fact.skip!=SkipTechnologyScreenObservation::zero || !w.game().preferences.CurrentTurn)return DiscoveryReactorError::stage_not_entered;
 if(!(w.game().preferences.TechOwners[tech]&(1u<<(unsigned)f)))return DiscoveryReactorError::award_context_required;
 DiscoveryReactorSequencePlan next;auto error=next.capture(w,limit);if(error!=DiscoveryReactorError::ok)return error;next.upgrade_=fact.upgrade_scratch;auto runtime=fact.allocation;const uint8_t bit=(uint8_t)(1u<<(unsigned)f);
 for(int32_t reactor=2;reactor<=4;++reactor){const auto pre=d.reactors[reactor-1].prerequisite;if(pre< -2 || pre>=89)return DiscoveryReactorError::bad_state;if(pre!=tech)continue;
  for(int32_t id=f*64;id<(f+1)*64;++id){const auto& p=*next.frame_.prototype((size_t)id);if(!(p.unit_flags&1u) || (p.obsolete_factions&bit) || p.reactor_id>=reactor)continue;
   if(fact.map_owner==f && !boundaryFacts)return DiscoveryReactorError::reactor_presentation_required;
   if(next.count_>=192)return DiscoveryReactorError::overflow;
   ExistingProposalRequest q{f,p.chassis_id,p.weapon_id,p.armor_id,reactor,(p.unit_flags&4u)?-2:-1,p.ability_flags,fact.proposal_mode,fact.expansion};DiscoveryReactorEvent event{id,reactor,-1,-1,0,next.frame_.faction((size_t)f)->unk_103,runtime.last_created,p.unit_flags};ExistingProposalResult existing{};
   const auto e=world_existing_prototype_proposal(next.frame_,v,r,d,q,existing);
   if(e==ExistingProposalError::ok){event.kind=(int32_t)existing.kind;event.returned=existing.id;next.apply_existing(existing);}
   else{UnreferencedAllocationPlan proposal;const auto allocation=world_unreferenced_prototype_allocation(next.frame_,v,r,d,q,runtime,labels,suffix,proposal);
    if(allocation==UnreferencedAllocationError::no_slot){event.kind=5;event.returned=-1;}
    else{error=mapped(allocation);if(error!=DiscoveryReactorError::ok)return error;error=next.apply_allocation(proposal,f);if(error!=DiscoveryReactorError::ok)return error;event.returned=proposal.allocation().id();event.kind=4;event.retired=proposal.retired();runtime.last_created=proposal.allocation().last_created();}
   }
   if(event.returned!=id){HumanPruningResult prune{};if(world_human_component_pruning(next.frame_,v,r,d,fact.allocation.multiplayer,f,event.returned,prune)!=HumanPruningError::ok)return DiscoveryReactorError::bad_state;next.apply_flags(prune);next.upgrade_=0;}
   event.history_cursor=next.frame_.faction((size_t)f)->unk_103;event.last_created=runtime.last_created;next.events_[next.count_++]=event;
  }
 }
 if(boundaryFacts && fact.map_owner==f && next.count_){
  next.boundary_=DiscoveryReactorBoundary::reactor_presentation;
  next.reactor_=next.events_[next.count_-1].reactor;
  const auto* name=d.reactors[next.reactor_-1].name;const auto* end=(const char*)memchr(name,0,128);
  if(!end)return DiscoveryReactorError::bad_state;
  memcpy(next.reactor_name_,name,(size_t)(end-name)+1);
  next.last_=runtime.last_created;next.cache_a_=boundaryFacts->first_ui_cache;next.cache_b_=boundaryFacts->second_ui_cache;
 }
 next.ready_=true;out.swap(next);return DiscoveryReactorError::ok;
}

DiscoveryReactorError world_discovery_reactor_sequence(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,int32_t f,int32_t tech,const DiscoveryReactorFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,DiscoveryReactorSequencePlan& out,size_t limit){return discovery_reactor_sequence_impl(w,v,r,d,f,tech,fact,labels,suffix,nullptr,out,limit);}
DiscoveryReactorError world_discovery_reactor_suspension(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,int32_t f,int32_t tech,const DiscoveryReactorFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,const DiscoveryReactorBoundaryFacts& boundary,DiscoveryReactorSequencePlan& out,size_t limit){return discovery_reactor_sequence_impl(w,v,r,d,f,tech,fact,labels,suffix,&boundary,out,limit);}
}
