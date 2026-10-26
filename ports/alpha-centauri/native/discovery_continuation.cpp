/* SPDX-License-Identifier: GPL-3.0-or-later. See discovery_continuation.h. */
#include "discovery_continuation.h"
#include "technology_effects.h"
#include "technology_effects_ranges.h"
#include "monument_effects.h"
#include <string.h>
namespace alpha {namespace {
bool world_alias(const OriginalWorld& w,const void* p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;return effects_range_overlap(p,n,&w,sizeof w) || w.overlaps_byte_storage(p,n) || effects_array_overlap(p,n,w.game().bunglist.data,w.game().bunglist.size,1) || effects_array_overlap(p,n,w.tile(0),w.game().map.info.tile_count,sizeof(OriginalMapTile)) || effects_array_overlap(p,n,w.vehicle(0),w.game().vehicle_count,sizeof(OriginalVehicle)) || effects_array_overlap(p,n,w.base(0),w.game().base_count,sizeof(OriginalBase)) || effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype)) || effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState)) || effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
bool DiscoveryContinuationPlan::overlaps(const void* p,size_t n)const{return detail::effects_range_overlap(p,n,this,sizeof *this) || (state_.ready() && world_alias(state_.frame_,p,n));}
void DiscoveryContinuationPlan::swap(DiscoveryContinuationPlan& p){state_.swap(p.state_);auto exchange=[](auto& a,auto& b){auto x=a;a=b;b=x;};exchange(captured_,p.captured_);exchange(shared_,p.shared_);exchange(boundary_,p.boundary_);exchange(faction_,p.faction_);exchange(technology_,p.technology_);exchange(ready_,p.ready_);}
DiscoveryContinuationError DiscoveryContinuationPlan::build(const OriginalWorld& w,Variant variant,const Rules& rules,const UnitDefinitions& units,int32_t f,int32_t tech,const DiscoveryReactorFacts& reactor,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,const DiscoveryReactorBoundaryFacts& cache,const DiscoveryContinuationFacts& runtime,size_t limit){
 if(runtime.runtime!=DiscoveryRuntimeObservation::supplied)return DiscoveryContinuationError::runtime_required;
 if(runtime.source< -2 || runtime.source>7 || runtime.share<0 || runtime.share>1 || runtime.first_ever<0 || runtime.first_ever>1)return DiscoveryContinuationError::bad_state;
 const size_t overhead=sizeof(*this)-sizeof(state_);
 const size_t stage_limit=limit>=overhead?limit-overhead:0;
 const auto reactor_error=world_discovery_reactor_suspension(w,variant,rules,units,f,tech,reactor,labels,suffix,cache,state_,stage_limit);
 if(reactor_error!=DiscoveryReactorError::ok)return DiscoveryContinuationError::reactor_held;
 captured_=runtime;faction_=f;technology_=tech;
 if(state_.boundary()==DiscoveryReactorBoundary::reactor_presentation){boundary_=DiscoveryContinuationBoundary::reactor_presentation;ready_=true;return DiscoveryContinuationError::ok;}
 if(reactor.map_owner==f || ((uint32_t)runtime.game_state&0x80u)){boundary_=DiscoveryContinuationBoundary::generic_presentation;ready_=true;return DiscoveryContinuationError::ok;}
 auto& frame=state_.frame_;auto& faction=frame.factions_[f];const uint8_t bit=(uint8_t)(1u<<(unsigned)f);const auto& target=rules.technologies[tech];
 if(target.flags&1u){MonumentEffectInput input{};input.variant=variant;input.monuments=frame.monuments_;input.size=monument_table_bytes;input.faction=f;input.local_faction=reactor.map_owner;input.event=2;input.year=runtime.game_year;input.faction_rule_flags=frame.definitions_[f].rule_flags;MonumentEffectResult effect{};
  const auto e=monument_effect(input,effect);if(e!=LogicError::ok || effect.presentation_required)return DiscoveryContinuationError::effect_held;
  memcpy(frame.monuments_+(size_t)f*monument_record_bytes,effect.record,monument_record_bytes);
 }
 if((target.flags&8u) && !(faction.player_flags&0x200u)){
  for(size_t i=0;i<frame.game().map.info.tile_count;++i){auto& t=frame.tiles_[i];t.visibility|=bit;t.visible_items[f-1]=t.items;}
  for(size_t i=0;i<frame.game().base_count;++i){auto& b=frame.bases_[i];b.visibility|=bit;b.factions_pop_size_intel[f]=b.pop_size;}
  faction.player_flags|=0x200u;
 }
 TechnologyEffectsResult effect{};const auto error=world_technology_effects(frame,rules,f,effect);
 if(error==LogicError::overflow)return DiscoveryContinuationError::overflow;
 if(error!=LogicError::ok)return DiscoveryContinuationError::effect_held;
 faction.tech_commerce_bonus=effect.commerce;faction.tech_fungus_nutrient=effect.fungus_nutrient;faction.tech_fungus_mineral=effect.fungus_mineral;faction.tech_fungus_energy=effect.fungus_energy;faction.tech_fungus_unused=effect.fungus_unused;
 const bool shareable=tech<88 && target.prerequisites[0]>=-1 && (target.prerequisites[1]>=-1 || target.prerequisites[0]==-1);
 if(shareable){const uint8_t owners=frame.game().preferences.TechOwners[tech];unsigned count=0;for(unsigned i=1;i<8;++i)count+=(owners&(1u<<i))!=0;
  const int32_t base=frame.game().preferences.SecretProjects[11];if(base>=0){if((size_t)base>=frame.game().base_count)return DiscoveryContinuationError::bad_state;const int32_t recipient=frame.bases_[base].faction_id;if(recipient<0 || recipient>7)return DiscoveryContinuationError::bad_state;
   if(recipient>0 && !(owners&(1u<<(unsigned)recipient)) && count>=3){shared_={recipient,tech,-1,0};boundary_=DiscoveryContinuationBoundary::datalinks_award;ready_=true;return DiscoveryContinuationError::ok;}
  }
  for(int32_t recipient=1;recipient<8;++recipient){const auto threshold=frame.definitions_[recipient].rule_sharetech;if(threshold>0 && !(owners&(1u<<(unsigned)recipient)) && count>=(uint32_t)threshold){shared_={recipient,tech,-2,1};boundary_=DiscoveryContinuationBoundary::techshare_award;ready_=true;return DiscoveryContinuationError::ok;}}
 }
 boundary_=DiscoveryContinuationBoundary::first_secrets_tail;ready_=true;return DiscoveryContinuationError::ok;
}
DiscoveryContinuationError world_discovery_continuation(const OriginalWorld& w,Variant variant,const Rules& rules,const UnitDefinitions& units,int32_t f,int32_t tech,const DiscoveryReactorFacts& reactor,const PrototypeNameLabels& labels,const PrototypeNameSuffix& suffix,const DiscoveryReactorBoundaryFacts& cache,const DiscoveryContinuationFacts& runtime,DiscoveryContinuationPlan& out,size_t limit){using detail::effects_range_overlap;
 const void* inputs[]={&rules,&units,&reactor,&labels,&suffix,&cache,&runtime};const size_t sizes[]={sizeof rules,sizeof units,sizeof reactor,sizeof labels,sizeof suffix,sizeof cache,sizeof runtime};
 if(world_alias(w,&out,sizeof out))return DiscoveryContinuationError::bad_state;
 for(unsigned i=0;i<7;++i)if(effects_range_overlap(&out,sizeof out,inputs[i],sizes[i]))return DiscoveryContinuationError::bad_state;
 if(out.overlaps(&w,sizeof w) || (w.loaded() && (out.overlaps(w.original_bytes(),w.original_size()) || out.overlaps(w.current_monuments().data,w.current_monuments().size) || out.overlaps(w.game().bunglist.data,w.game().bunglist.size) || out.overlaps(w.tile(0),w.game().map.info.tile_count*sizeof(OriginalMapTile)) || out.overlaps(w.vehicle(0),w.game().vehicle_count*sizeof(OriginalVehicle)) || out.overlaps(w.base(0),w.game().base_count*sizeof(OriginalBase)) || out.overlaps(w.prototype(0),512*sizeof(OriginalPrototype)) || out.overlaps(w.faction(0),8*sizeof(OriginalFactionState)) || out.overlaps(w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))))return DiscoveryContinuationError::bad_state;
 for(unsigned i=0;i<7;++i)if(out.overlaps(inputs[i],sizes[i]))return DiscoveryContinuationError::bad_state;
 DiscoveryContinuationPlan next;const auto error=next.build(w,variant,rules,units,f,tech,reactor,labels,suffix,cache,runtime,limit);if(error!=DiscoveryContinuationError::ok)return error;out.swap(next);return DiscoveryContinuationError::ok;
}
}
