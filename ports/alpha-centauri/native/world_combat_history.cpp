/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_history.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
CombatHistoryError WorldCombatHistoryPlan::prepare(const OriginalWorld&w,const CombatHistoryFacts&f,size_t limit){
 using E=CombatHistoryError;if(f.observation!=EngagementObservation::supplied||(uint32_t)f.edition>1||f.attacker_design>=512||f.defender_design>=512||f.attacker_faction>7||f.defender_faction>7||f.interlude_pending>1)return E::bad_state;
 if(f.interlude_pending||(f.edition==Variant::alien_crossfire)!=(w.game().data_version>=11))return E::unsupported;
 const auto*a=w.prototype(f.attacker_design);const auto*d=w.prototype(f.defender_design);if(!a||!d)return E::bad_state;
 constexpr size_t extra=sizeof(WorldCombatHistoryPlan)-sizeof(LandEngagementPlan);if(limit<extra)return E::out_of_memory;const auto captured=storage_.capture(w,limit-extra);if(captured!=EngagementError::ok)return captured==EngagementError::out_of_memory?E::out_of_memory:E::bad_state;
 auto&ap=storage_.frame_.prototypes_[f.attacker_design];auto&dp=storage_.frame_.prototypes_[f.defender_design];ap.combat_factions=(uint8_t)(ap.combat_factions|(1u<<f.defender_faction));dp.combat_factions=(uint8_t)(dp.combat_factions|(1u<<f.attacker_faction));summary_={f.attacker_design,f.defender_design,f.attacker_faction,f.defender_faction,a->combat_factions,d->combat_factions,ap.combat_factions,dp.combat_factions,f.attacker_design==f.defender_design?1u:0u};ready_=true;return E::ok;
}
void WorldCombatHistoryPlan::swap(WorldCombatHistoryPlan&p){storage_.swap(p.storage_);auto s=summary_;summary_=p.summary_;p.summary_=s;bool b=ready_;ready_=p.ready_;p.ready_=b;}
CombatHistoryError stage_world_combat_history(const OriginalWorld&w,const CombatHistoryFacts&f,WorldCombatHistoryPlan&out,size_t limit){
 using detail::effects_range_overlap;using E=CombatHistoryError;if(!w.loaded()||alias(w,&out,sizeof out)||alias(w,&f,sizeof f)||effects_range_overlap(&f,sizeof f,&out,sizeof out))return E::bad_state;if(const auto*owned=out.frame())if(alias(*owned,&f,sizeof f))return E::bad_state;WorldCombatHistoryPlan next;const auto e=next.prepare(w,f,limit);if(e==E::ok)out.swap(next);return e;
}
}
