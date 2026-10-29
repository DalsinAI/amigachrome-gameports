/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_prebattle_territory.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
PrebattleTerritoryError inspect_world_prebattle_territory(const OriginalWorld&w,size_t a,size_t b,const WorldPrebattleTerritoryRuntime&r,WorldPrebattleTerritoryProposal&out){
 using E=PrebattleTerritoryError;
 if(!w.loaded()||a==b||alias(w,&out,sizeof out)||alias(w,&r,sizeof r)||detail::effects_range_overlap(&r,sizeof r,&out,sizeof out))return E::bad_state;
 if(r.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if(r.current_faction>7)return E::bad_state;
 const auto*av=w.vehicle(a);const auto*bv=w.vehicle(b);
 if(!av||!bv||av->faction_id>7||bv->faction_id>7)return E::bad_state;
 if(!av->faction_id||!bv->faction_id||av->faction_id==bv->faction_id)return E::unsupported;
 if(av->unit_id<0||av->unit_id>=512||bv->unit_id<0||bv->unit_id>=512)return E::bad_state;
 const auto*ap=w.prototype((size_t)av->unit_id);const auto*bp=w.prototype((size_t)bv->unit_id);if(!ap||!bp)return E::bad_state;
 if(ap->plan==11||bp->plan==12)return E::unsupported;
 const auto&m=w.game().map.info;
 if(av->x<0||av->y<0||(uint32_t)av->x>=m.width||(uint32_t)av->y>=m.height||((av->x^av->y)&1))return E::bad_state;
 const size_t index=(size_t)av->y*(m.width/2)+(uint32_t)av->x/2;const auto*t=w.tile(index);if(!t)return E::bad_state;
 if(t->owner<0||t->owner>7)return E::unsupported;
 const auto*owner=w.faction((size_t)t->owner);const auto*af=w.faction(av->faction_id);const auto*bf=w.faction(bv->faction_id);if(!owner||!af||!bf)return E::bad_state;
 const PrebattleTerritoryFacts f{BattleEntryObservation::supplied,w.game().data_version>=11?Variant::alien_crossfire:Variant::base,av->faction_id,bv->faction_id,r.current_faction,t->owner,af->diplo_status[(size_t)t->owner],bf->diplo_status[(size_t)t->owner],owner->diplo_status[bv->faction_id],owner->mil_strength_1,bf->mil_strength_1,bf->diplo_friction[(size_t)t->owner],r.negotiation_faction,r.negotiation_other,r.negotiation_accumulator};
 PrebattleTerritoryProposal effect{};const auto e=inspect_prebattle_territory(f,effect);if(e!=E::ok)return e;
 const WorldPrebattleTerritoryProposal next{(uint32_t)index,t->owner,av->faction_id,bv->faction_id,effect};out=next;return E::ok;
}
}
