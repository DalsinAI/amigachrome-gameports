/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_battle_entry.h"
#include "world_unit_stats.h"
#include "movement.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
WorldBattleEntryError mapped(LogicError e){return e==LogicError::overflow?WorldBattleEntryError::overflow:WorldBattleEntryError::bad_state;}
}
WorldBattleEntryError inspect_world_battle_entry(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,const WorldBattleEntryRuntime&runtime,WorldBattleEntryInspection&out){
 using E=WorldBattleEntryError;using detail::effects_range_overlap;
 if(!w.loaded()||attacker==defender||alias(w,&out,sizeof out))return E::bad_state;
 const void*inputs[]={&d,&t,&r,&runtime};const size_t sizes[]={sizeof d,sizeof t,sizeof r,sizeof runtime};
 for(unsigned i=0;i<4;++i){if(alias(w,inputs[i],sizes[i])||effects_range_overlap(inputs[i],sizes[i],&out,sizeof out))return E::bad_state;for(unsigned j=0;j<i;++j)if(effects_range_overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return E::bad_state;}
 if(runtime.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if(runtime.attacker_report_count<0||runtime.attacker_report_count>4||runtime.multiplayer_active>1)return E::bad_state;
 if(runtime.attack_flags!=2||runtime.mode!=0||runtime.execution_argument!=1||runtime.multiplayer_active)return E::unsupported;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return E::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp)return E::bad_state;
 if(!a->faction_id||!b->faction_id||ap->plan<0||ap->plan>3||bp->plan<0||bp->plan>3||ap->ability_flags)return E::unsupported;
 if(w.game().data_version>=11&&a->unit_id>=64){const auto*f=w.faction_definition((size_t)a->unit_id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return E::bad_state;for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15)return E::unsupported;}
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t tile;auto e=movement_tile_index(geometry,b->x,b->y,tile);if(e!=LogicError::ok)return mapped(e);const auto*cell=w.tile(tile);if(!cell)return E::bad_state;
 if(cell->items&1u)return E::unsupported;
 for(size_t i=0;i<w.game().base_count;++i){const auto*base=w.base(i);if(base->x==b->x&&base->y==b->y)return E::unsupported;}
 WorldBattleEntryInspection next{};e=world_land_combat_strengths(w,d,t,r,attacker,defender,next.before_hasty);if(e!=LogicError::ok)return mapped(e);
 e=world_vehicle_speed(w,d,t,attacker,r.move_rate_roads,false,next.speed);if(e!=LogicError::ok)return mapped(e);
 const Variant edition=w.game().data_version>=11?Variant::alien_crossfire:Variant::base;
 BattleChargeFacts cf{BattleEntryObservation::supplied,edition,0,r.move_rate_roads,next.speed,a->moves_spent};
 if(inspect_battle_charge(cf,next.charge)!=BattleEntryError::ok)return E::bad_state;
 BattleHastyFacts hf{BattleEntryObservation::supplied,edition,runtime.attack_flags,a->state,b->state,0,a->faction_id,(uint32_t)a->unit_id,a->moves_spent,next.speed,0,r.move_rate_roads,next.charge.charge,next.before_hasty.offense,runtime.attacker_report_count};
 if(inspect_battle_hasty(hf,next.hasty)!=BattleEntryError::ok)return E::bad_state;
 out=next;return E::ok;
}
}
