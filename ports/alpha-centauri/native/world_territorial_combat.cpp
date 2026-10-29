/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_territorial_combat.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
}
WorldTerritoryError WorldTerritorialCombatPlan::prepare(const OriginalWorld&w,size_t a,size_t b,const TerritoryRuntime&run,size_t limit){
 using E=WorldTerritoryError;
 if(run.observation!=TerritoryObservation::supplied)return E::missing_runtime;
 if((uint32_t)run.edition>1||run.multiplayer_active>1||run.pbem_active>1)return E::bad_state;
 if(run.multiplayer_active||run.pbem_active||(run.edition==Variant::alien_crossfire)!=(w.game().data_version>=11))return E::unsupported;
 const auto*av=w.vehicle(a);const auto*dv=w.vehicle(b);const auto&m=w.game().map.info;
 if(av->faction_id>7||dv->faction_id>7||av->faction_id==dv->faction_id||dv->x<0||dv->y<0||(uint32_t)dv->x>=m.width||(uint32_t)dv->y>=m.height||((dv->x^dv->y)&1))return E::bad_state;
 const size_t index=(size_t)dv->y*(m.width/2)+(uint32_t)dv->x/2;const auto*t=w.tile(index);if(!t)return E::bad_state;
 const bool land=(t->climate&0xe0u)>=0x60u;
 if(land&&(t->owner<0||t->owner>7))return E::unsupported;
 const auto*owner=t->owner>=0&&t->owner<8?w.faction((size_t)t->owner):nullptr;const auto*attacker=w.faction(av->faction_id);
 // Water never indexes the original owner's faction. Explicit unused facts
 // cannot become authority for an unsupported land owner.
 const TerritorialCombatFacts facts{run.observation,run.edition,t->climate,t->owner,av->faction_id,dv->faction_id,land?owner->diplo_status[dv->faction_id]:0u,land?owner->diplo_status[av->faction_id]:0u,land?attacker->diplo_gifts[(size_t)t->owner]:0,land?owner->diplo_friction[av->faction_id]:0,run.negotiation_faction,run.negotiation_other,run.negotiation_accumulator};
 TerritorialCombatResult effect{};const auto checked=inspect_territorial_combat(facts,effect);if(checked!=TerritoryError::ok)return checked==TerritoryError::unsupported?E::unsupported:checked==TerritoryError::missing_runtime?E::missing_runtime:E::bad_state;
 constexpr size_t extra=sizeof(WorldTerritorialCombatPlan)-sizeof(LandEngagementPlan);if(limit<extra)return E::out_of_memory;
 const auto captured=storage_.capture(w,limit-extra);if(captured!=EngagementError::ok)return captured==EngagementError::out_of_memory?E::out_of_memory:E::bad_state;
 if(effect.applied){storage_.frame_.factions_[av->faction_id].diplo_gifts[(size_t)t->owner]=effect.gift_after;storage_.frame_.factions_[(size_t)t->owner].diplo_friction[av->faction_id]=effect.friction_after;}
 summary_={(uint32_t)index,t->owner,av->faction_id,dv->faction_id,effect};runtime_=run;runtime_.negotiation_accumulator=effect.negotiation_after;ready_=true;return E::ok;
}
void WorldTerritorialCombatPlan::swap(WorldTerritorialCombatPlan&p){storage_.swap(p.storage_);auto s=summary_;summary_=p.summary_;p.summary_=s;auto r=runtime_;runtime_=p.runtime_;p.runtime_=r;bool b=ready_;ready_=p.ready_;p.ready_=b;}
WorldTerritoryError stage_world_territorial_combat(const OriginalWorld&w,size_t a,size_t b,const TerritoryRuntime&run,WorldTerritorialCombatPlan&out,size_t limit){
 using detail::effects_range_overlap;using E=WorldTerritoryError;
 if(!w.loaded()||a==b||!w.vehicle(a)||!w.vehicle(b)||alias(w,&out,sizeof out)||alias(w,&run,sizeof run)||effects_range_overlap(&out,sizeof out,&run,sizeof run))return E::bad_state;
 if(const auto*owned=out.frame())if(alias(*owned,&run,sizeof run))return E::bad_state;
 WorldTerritorialCombatPlan next;const auto e=next.prepare(w,a,b,run,limit);if(e==E::ok)out.swap(next);return e;
}
}
