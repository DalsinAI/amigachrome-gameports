/* SPDX-License-Identifier: GPL-3.0-or-later. See discovery_reactor_stage.h. */
#include "discovery_reactor_stage.h"
#include "technology_effects_ranges.h"
namespace alpha {
void DiscoveryReactorPlan::swap(DiscoveryReactorPlan& p){proposal_.swap(p.proposal_);auto count=count_;count_=p.count_;p.count_=count;auto e=existing_;existing_=p.existing_;p.existing_=e;bool early=early_;early_=p.early_;p.early_=early;auto s=selected_;selected_=p.selected_;p.selected_=s;for(unsigned i=0;i<512;++i){auto n=flags_[i];flags_[i]=p.flags_[i];p.flags_[i]=n;}auto n=source_;source_=p.source_;p.source_=n;n=reactor_;reactor_=p.reactor_;p.reactor_=n;n=upgrade_;upgrade_=p.upgrade_;p.upgrade_=n;bool b=ready_;ready_=p.ready_;p.ready_=b;b=has_;has_=p.has_;p.has_=b;}
static bool alias(const OriginalWorld& w,const void* a,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(a,n,&w,sizeof w) || w.overlaps_byte_storage(a,n) || effects_array_overlap(a,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(a,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(a,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(a,n,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(a,n,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(a,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
DiscoveryReactorError world_discovery_reactor_stage(const OriginalWorld& w,Variant v,const Rules& r,const UnitDefinitions& d,int32_t f,int32_t tech,const DiscoveryReactorFacts& fact,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,DiscoveryReactorPlan& out){using detail::effects_range_overlap;
 auto overlaps=[&](const void* a,size_t n){return alias(w,a,n) || effects_range_overlap(a,n,&r,sizeof r) || effects_range_overlap(a,n,&d,sizeof d) || effects_range_overlap(a,n,&fact,sizeof fact) || effects_range_overlap(a,n,&labels,sizeof labels) || effects_range_overlap(a,n,&suffix,sizeof suffix);};
 if(overlaps(&out,sizeof out) || !w.loaded() || f<1 || f>7 || tech<0 || tech>=89 || (v!=Variant::base && v!=Variant::alien_crossfire) || ((w.game().data_version<11)!=(v==Variant::base)) || r.count!=89)return DiscoveryReactorError::bad_state;
 const auto& old=out.proposal_.allocation();if(old.count() && (old.count()>512 || overlaps(old.base(0),old.count()*sizeof(OriginalBase))))return DiscoveryReactorError::bad_state;
 if(v!=Variant::base)return DiscoveryReactorError::naming_context_required;
 if(fact.runtime!=DiscoveryRuntimeObservation::supplied || (fact.skip!=SkipTechnologyScreenObservation::zero && fact.skip!=SkipTechnologyScreenObservation::nonzero) || fact.map_owner<0 || fact.map_owner>7 || fact.proposal_mode!=ProposalMode::normal || fact.allocation.multiplayer!=PruningRuntime::multiplayer_off || fact.allocation.cursor!=AllocationCursorObservation::supplied || !(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)f)) || (w.game().preferences.GameMorePreferences&0x100000) || (w.game().preferences.GamePreferences&0x4000))return DiscoveryReactorError::runtime_required;
 if(fact.skip!=SkipTechnologyScreenObservation::zero || !w.game().preferences.CurrentTurn)return DiscoveryReactorError::stage_not_entered;
 if(!(w.game().preferences.TechOwners[tech]&(1u<<(unsigned)f)))return DiscoveryReactorError::award_context_required;
 int32_t source=-1,target=0;unsigned pairs=0;const uint8_t bit=(uint8_t)(1u<<(unsigned)f);
 for(int32_t reactor=2;reactor<=4;++reactor){const auto prereq=d.reactors[reactor-1].prerequisite;if(prereq<-2 || prereq>=89)return DiscoveryReactorError::bad_state;if(prereq!=tech)continue;for(int32_t id=f*64;id<(f+1)*64;++id){const auto* p=w.prototype((size_t)id);if(!p)return DiscoveryReactorError::bad_state;if((p->unit_flags&1u) && !(p->obsolete_factions&bit) && p->reactor_id<reactor){++pairs;source=id;target=reactor;}}}
 if(pairs && fact.map_owner==f)return DiscoveryReactorError::reactor_presentation_required;
 DiscoveryReactorPlan next;next.source_=source;next.reactor_=target;next.upgrade_=fact.upgrade_scratch;
 if(pairs>1){
  HumanPruningResult staged{};for(unsigned i=0;i<512;++i)staged.flags[i]=w.prototype(i)->unit_flags;
  for(int32_t reactor=2;reactor<=4;++reactor){if(d.reactors[reactor-1].prerequisite!=tech)continue;
   for(int32_t id=f*64;id<(f+1)*64;++id){const auto& p=*w.prototype((size_t)id);if(!(staged.flags[id]&1u) || (p.obsolete_factions&bit) || p.reactor_id>=reactor)continue;
    ExistingProposalRequest q{f,p.chassis_id,p.weapon_id,p.armor_id,reactor,(staged.flags[id]&4u)?-2:-1,p.ability_flags,fact.proposal_mode,fact.expansion};ExistingProposalResult result{};
    const auto e=world_existing_prototype_proposal(w,v,r,d,q,result);
    // Only these returns leave every record/history/queue unchanged. Their
    // outcome is independent of the monotone4/100 coverage additions, including
    // the derived plan-2 request bit. Any changing proposal needs a full staged
    // World context and rejects this whole sequence, never reuses a stale one.
    if(e!=ExistingProposalError::ok || result.kind==ExistingProposalKind::reused)return DiscoveryReactorError::ordered_sequence_required;
    HumanPruningResult after{};if(world_human_component_pruning_prior(w,v,r,d,fact.allocation.multiplayer,f,result.id,staged,after)!=HumanPruningError::ok)return DiscoveryReactorError::bad_state;
    staged=after;next.existing_=result;next.source_=id;next.reactor_=reactor;next.early_=true;next.has_=true;++next.count_;next.upgrade_=0;
   }
  }
  if(next.count_!=pairs)return DiscoveryReactorError::bad_state;
  for(unsigned i=0;i<512;++i)next.flags_[i]=staged.flags[i];
 }else if(pairs){next.count_=1;const auto& p=*w.prototype((size_t)source);ExistingProposalRequest q{f,p.chassis_id,p.weapon_id,p.armor_id,target,(p.unit_flags&4u)?-2:-1,p.ability_flags,fact.proposal_mode,fact.expansion};ExistingProposalResult existing{};const auto existingError=world_existing_prototype_proposal(w,v,r,d,q,existing);
  if(existingError==ExistingProposalError::ok){
   next.early_=true;next.existing_=existing;next.has_=true;
   for(unsigned i=0;i<512;++i)next.flags_[i]=w.prototype(i)->unit_flags;
   if(existing.kind==ExistingProposalKind::reused){next.selected_=existing.prototype;next.flags_[existing.id]=existing.prototype.unit_flags;}
   if(existing.id!=source){HumanPruningResult repeated{};if(world_human_component_pruning_existing(w,v,r,d,fact.allocation.multiplayer,q,repeated)!=HumanPruningError::ok)return DiscoveryReactorError::bad_state;for(unsigned i=0;i<512;++i)next.flags_[i]=repeated.flags[i];if(existing.kind==ExistingProposalKind::reused)next.selected_.unit_flags=repeated.flags[existing.id];next.upgrade_=0;}
  }else {
  const auto e=world_unreferenced_prototype_allocation(w,v,r,d,q,fact.allocation,labels,suffix,next.proposal_);
  if(e!=UnreferencedAllocationError::ok){switch(e){case UnreferencedAllocationError::runtime_required:return DiscoveryReactorError::runtime_required;case UnreferencedAllocationError::existing_return_required:case UnreferencedAllocationError::no_slot:return DiscoveryReactorError::proposal_required;case UnreferencedAllocationError::strength_required:return DiscoveryReactorError::proposal_required;case UnreferencedAllocationError::live_reference_required:return DiscoveryReactorError::live_reference_required;case UnreferencedAllocationError::naming_context_required:return DiscoveryReactorError::naming_context_required;case UnreferencedAllocationError::naming_nonreturning:return DiscoveryReactorError::naming_nonreturning;case UnreferencedAllocationError::overflow:return DiscoveryReactorError::overflow;case UnreferencedAllocationError::out_of_memory:return DiscoveryReactorError::out_of_memory;default:return DiscoveryReactorError::bad_state;}}
  const auto& result=next.proposal_.allocation();next.selected_=result.selected();HumanPruningResult prior{};for(unsigned i=0;i<512;++i)prior.flags[i]=result.flags(i);
  if(result.id()!=source){HumanPruningResult repeated{};if(world_human_component_pruning_after(w,v,r,d,fact.allocation.multiplayer,f,result.id(),next.selected_,prior,repeated)!=HumanPruningError::ok)return DiscoveryReactorError::bad_state;for(unsigned i=0;i<512;++i)next.flags_[i]=repeated.flags[i];next.selected_.unit_flags=repeated.flags[result.id()];next.upgrade_=0;}else for(unsigned i=0;i<512;++i)next.flags_[i]=prior.flags[i];next.has_=true;
  }
 }else for(unsigned i=0;i<512;++i)next.flags_[i]=w.prototype(i)->unit_flags;
 next.ready_=true;out.swap(next);return DiscoveryReactorError::ok;
}
}
