/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_battle_reporting.h"
#include "fungus_combat.h"
#include "world_combat_factions.h"
#include "ground_modifiers.h"
#include "movement.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool alias(const OriginalWorld&w,const void*p,size_t n){using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();return effects_range_overlap(p,n,&w,sizeof w)||w.overlaps_byte_storage(p,n)||effects_array_overlap(p,n,g.bunglist.data,g.bunglist.size,1)||effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))||effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))||effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))||effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))||effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))||effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));}
WorldBattleEntryError mapped(LogicError e){return e==LogicError::overflow?WorldBattleEntryError::overflow:WorldBattleEntryError::bad_state;}
}
WorldBattleEntryError inspect_world_battle_reporting(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,const WorldBattleReportingRuntime&runtime,const AttackModifierLedger&prior,WorldBattleReportingInspection&out){
 using E=WorldBattleEntryError;using detail::effects_range_overlap;
 if(!w.loaded()||attacker==defender||alias(w,&out,sizeof out))return E::bad_state;
 const void*inputs[]={&d,&t,&r,&runtime,&prior};const size_t sizes[]={sizeof d,sizeof t,sizeof r,sizeof runtime,sizeof prior};
 for(unsigned i=0;i<5;++i){if(alias(w,inputs[i],sizes[i])||effects_range_overlap(inputs[i],sizes[i],&out,sizeof out))return E::bad_state;for(unsigned j=0;j<i;++j)if(effects_range_overlap(inputs[i],sizes[i],inputs[j],sizes[j]))return E::bad_state;}
 if(runtime.observation!=BattleEntryObservation::supplied)return E::missing_runtime;
 if(runtime.multiplayer_active>1)return E::bad_state;
 if(runtime.attack_flags!=2||runtime.mode!=0||runtime.execution_argument!=1||runtime.multiplayer_active)return E::unsupported;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return E::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp)return E::bad_state;
 if(!a->faction_id||!b->faction_id||ap->plan<0||ap->plan>3||bp->plan<0||bp->plan>3||ap->ability_flags)return E::unsupported;
 if(w.game().data_version>=11&&a->unit_id>=64){const auto*f=w.faction_definition((size_t)a->unit_id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return E::bad_state;for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15)return E::unsupported;}
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t tile;auto e=movement_tile_index(geometry,b->x,b->y,tile);if(e!=LogicError::ok)return mapped(e);const auto*cell=w.tile(tile);if(!cell)return E::bad_state;if(cell->items&1u)return E::unsupported;
 for(size_t i=0;i<w.game().base_count;++i){const auto*base=w.base(i);if(base->x==b->x&&base->y==b->y)return E::unsupported;}
 BasicCombatStrengths attack{},defend{};e=world_basic_combat_strengths(w,d,t,r,attacker,defender,false,false,false,attack);if(e!=LogicError::ok)return mapped(e);e=world_basic_combat_strengths(w,d,t,r,defender,attacker,false,false,false,defend);if(e!=LogicError::ok)return mapped(e);
 FungusCombatStrength fungus{};e=world_fungus_combat_adjustment(w,d,attacker,defender,attack.offense,fungus);if(e!=LogicError::ok)return mapped(e);
 CombatFactionStrengths faction{};e=world_combat_faction_modifiers(w,d,t,r,attacker,defender,fungus.offense,defend.defense,faction);if(e!=LogicError::ok)return mapped(e);
 GroundModifierStrengths ground{};e=world_ground_combat_modifiers(w,d,t,r,attacker,defender,faction.offense,faction.defense,ground);if(e!=LogicError::ok)return mapped(e);
 const Variant edition=w.game().data_version>=11?Variant::alien_crossfire:Variant::base;const auto*fd=w.faction_definition(a->faction_id);if(!fd||fd->faction_bonus_count<0||fd->faction_bonus_count>8)return E::bad_state;
 AttackModifierFacts f{};f.observation=BattleEntryObservation::supplied;f.edition=edition;f.fungus_applied=fungus.applied;f.fanatic_flag=(fd->rule_flags&0x400u)?1:0;f.fanatic_percent=r.combat_pct_fanatic_atk_bonus;f.ground_applied=ground.applied;f.mobile_percent=r.combat_pct_mobile_open_ground;f.road_percent=r.combat_pct_atk_road;f.downhill_percent=r.combat_pct_atk_higher_elev;
 if(edition==Variant::alien_crossfire)for(int i=0;i<fd->faction_bonus_count;++i)if(fd->faction_bonus_id[i]==18)f.offense_factors[f.offense_factor_count++]=fd->faction_bonus_val1[i];
 WorldBattleReportingInspection next{};if(attack_modifier_ledger(f,prior,next.modifiers)!=BattleEntryError::ok)return E::bad_state;
 const WorldBattleEntryRuntime observed{runtime.observation,runtime.attack_flags,runtime.mode,runtime.execution_argument,runtime.multiplayer_active,(int32_t)next.modifiers.ledger.count};
 const auto result=inspect_world_battle_entry(w,d,t,r,attacker,defender,observed,next.entry);if(result!=E::ok)return result;
 next.final_ledger=next.modifiers.ledger;if(next.entry.hasty.report_append_required){if(next.final_ledger.count>=4)return E::bad_state;next.final_ledger.slots[next.final_ledger.count++]={339,next.entry.hasty.penalty};}
 next.lookup_count=next.modifiers.lookup_count+next.entry.hasty.string_lookup_required;out=next;return E::ok;
}
}
