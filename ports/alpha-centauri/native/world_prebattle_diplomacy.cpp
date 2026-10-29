/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_prebattle_diplomacy.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
PrebattleNotificationError inspect_world_prebattle_notification(const OriginalWorld&w,size_t a,size_t b,const WorldPrebattleNotificationRuntime&r,PrebattleNotificationProposal&out){
 using E=PrebattleNotificationError;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out)||alias(w,&r,sizeof r)||detail::effects_range_overlap(&r,sizeof r,&out,sizeof out))return E::bad_state;
 if(r.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 const auto*av=w.vehicle(a);const auto*bv=w.vehicle(b);
 if(!av||!bv||av->faction_id>7||bv->faction_id>7)return E::bad_state;
 if(!av->faction_id||!bv->faction_id)return E::unsupported;
 const auto*bf=w.faction(bv->faction_id);if(!bf)return E::bad_state;
 PrebattleNotificationFacts f{BattleEntryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,av->faction_id,bv->faction_id,r.current_faction,r.human_status,r.game_flags,r.popup_flag,r.turn,r.prior_warning,(uint32_t)bf->diplo_spoke[av->faction_id],{}};
 for(unsigned i=0;i<8;++i){const auto*source=w.faction(i);if(!source)return E::bad_state;for(unsigned j=0;j<8;++j)f.status[i*8+j]=source->diplo_status[j];}
 return inspect_prebattle_notification(f,out);
}
AggressionError inspect_world_prebattle_aggression(const OriginalWorld&w,size_t a,size_t b,const WorldPrebattleAggressionRuntime&r,PrebattleAggressionProposal&out){
 using E=AggressionError;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out)||alias(w,&r,sizeof r)||detail::effects_range_overlap(&r,sizeof r,&out,sizeof out))return E::bad_state;
 if(r.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 const auto*av=w.vehicle(a);const auto*bv=w.vehicle(b);
 if(!av||!bv||av->faction_id>7||bv->faction_id>7)return E::bad_state;
 if(!av->faction_id||!bv->faction_id)return E::unsupported;
 const auto*af=w.faction(av->faction_id);const auto*bf=w.faction(bv->faction_id);if(!af||!bf)return E::bad_state;
 const PrebattleAggressionFacts f{BattleEntryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,av->faction_id,bv->faction_id,r.current_faction,af->diplo_status[bv->faction_id],bf->diplo_status[av->faction_id],af->diplo_combat_count[bv->faction_id],bf->diplo_combat_count[av->faction_id]};
 return inspect_prebattle_aggression(f,out);
}
}
