/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_combat_difficulty.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
BattleEntryError inspect_world_combat_difficulty(const OriginalWorld&w,size_t a,size_t b,const WorldCombatDifficultyRuntime&runtime,CombatDifficultyResult&out){
 using E=BattleEntryError;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out)||alias(w,&runtime,sizeof runtime)||detail::effects_range_overlap(&runtime,sizeof runtime,&out,sizeof out))return E::bad_state;
 if(runtime.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 const auto*av=w.vehicle(a);const auto*bv=w.vehicle(b);if(!av||!bv||av->faction_id>7||bv->faction_id>7)return E::bad_state;
 if(!av->faction_id||!bv->faction_id)return E::unsupported;
 const auto*af=w.faction(av->faction_id);const auto*bf=w.faction(bv->faction_id);if(!af||!bf)return E::bad_state;
 const CombatDifficultyFacts f{BattleEntryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,av->faction_id,bv->faction_id,runtime.human_status,af->diff_level,bf->diff_level,runtime.offense};
 return inspect_combat_difficulty(f,out);
}
}
