/* SPDX-License-Identifier: GPL-3.0-or-later. See fungus_combat.h. */
#include "land_combat.h"
#include "fungus_combat.h"
#include "combat_factions.h"
#include "world_combat_factions.h"
#include "ground_modifiers.h"
#include "base_combat.h"
#include "sensor_combat.h"
#include "world_air_port_combat.h"
#include "world_defense_abilities.h"
#include "world_unit_stats.h"
#include "saved_speed.h"
#include "technology_effects_ranges.h"
namespace alpha {namespace {
bool disjoint(const OriginalWorld&w,const void*p,size_t n){
 using detail::effects_range_overlap;using detail::effects_array_overlap;const auto&g=w.game();
 return !effects_range_overlap(p,n,&w,sizeof w)&&!w.overlaps_byte_storage(p,n)&&
 !effects_array_overlap(p,n,w.tile(0),g.map.info.tile_count,sizeof(OriginalMapTile))&&
 !effects_array_overlap(p,n,w.vehicle(0),g.vehicle_count,sizeof(OriginalVehicle))&&
 !effects_array_overlap(p,n,w.base(0),g.base_count,sizeof(OriginalBase))&&
 !effects_array_overlap(p,n,w.prototype(0),512,sizeof(OriginalPrototype))&&
 !effects_array_overlap(p,n,w.faction(0),8,sizeof(OriginalFactionState))&&
 !effects_array_overlap(p,n,w.faction_definition(0),8,sizeof(OriginalFactionDefinition));
}

LogicError blink(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,size_t id,bool&out){
 const auto*p=w.prototype(id);if(!p)return LogicError::bad_state;if(p->ability_flags&0x20000u){out=true;return LogicError::ok;}
 if(w.game().data_version<11||id<64){out=false;return LogicError::ok;}const auto*f=w.faction_definition(id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){const int a=f->faction_bonus_val1[i];if(a<0||(uint32_t)a>=d.ability_count)return LogicError::bad_state;if(a!=17)continue;bool known=false;const auto e=world_has_technology(w,t,d.abilities[a].prerequisite,(int32_t)(id/64),known);if(e!=LogicError::ok)return e;if(known){out=true;return LogicError::ok;}}
 out=false;return LogicError::ok;
}

}
LogicError world_land_combat_strengths(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,LandCombatStrengths&out){
 using detail::effects_range_overlap;
 if(!w.loaded()||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp||ap->chassis_id>=9||bp->chassis_id>=9||ap->weapon_id>=d.weapon_count||bp->armor_id>=d.armor_count||ap->plan==11)return LogicError::bad_state;
 if(d.chassis[ap->chassis_id].triad||d.chassis[bp->chassis_id].triad||d.chassis[ap->chassis_id].missile||d.chassis[bp->chassis_id].missile||d.weapons[ap->weapon_id].offense<0||d.armor[bp->armor_id].defense<0)return LogicError::bad_state;
 BasicCombatStrengths attack,defend;auto e=world_basic_combat_strengths(w,d,t,r,attacker,defender,false,false,false,attack);if(e!=LogicError::ok)return e;e=world_basic_combat_strengths(w,d,t,r,defender,attacker,false,false,false,defend);if(e!=LogicError::ok)return e;
 FungusCombatStrength fungus;e=world_fungus_combat_adjustment(w,d,attacker,defender,attack.offense,fungus);if(e!=LogicError::ok)return e;
 CombatFactionStrengths faction;e=world_combat_faction_modifiers(w,d,t,r,attacker,defender,fungus.offense,defend.defense,faction);if(e!=LogicError::ok)return e;
 GroundModifierStrengths ground;e=world_ground_combat_modifiers(w,d,t,r,attacker,defender,faction.offense,faction.defense,ground);if(e!=LogicError::ok)return e;
 TerrainDefense terrain;e=world_terrain_defense(w,d,defender,(int32_t)attacker,terrain);if(e!=LogicError::ok)return e;
 const int64_t value=(int64_t)ground.defense*terrain.multiplier;if(value<(-2147483647LL-1)||value>2147483647LL)return LogicError::overflow;int32_t offense=ground.offense,defense=(int32_t)value;
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t index;e=movement_tile_index(geometry,b->x,b->y,index);if(e!=LogicError::ok)return e;const auto*tile=w.tile(index);if(!tile)return LogicError::bad_state;
 uint32_t multiplier=2;
 if(tile->items&1u){BaseCombatAdjustment base;e=world_base_combat_adjustment(w,d,t,r,defender,(int32_t)attacker,offense,defense,base);if(e!=LogicError::ok)return e;offense=base.offense;defense=base.defense;multiplier=base.multiplier;}
 else{multiplier=(tile->items&0x800u)?3u:2u;bool bypass=false;e=blink(w,d,t,(size_t)a->unit_id,bypass);if(e!=LogicError::ok)return e;if(bypass&&multiplier>2)multiplier=2;const int64_t product=(int64_t)defense*multiplier;if(product<(-2147483647LL-1)||product>2147483647LL)return LogicError::overflow;defense=(int32_t)product;}
 SensorCombatAdjustment sensor;e=world_sensor_combat_adjustment(w,r,defender,defense,sensor);if(e!=LogicError::ok)return e;
 AirPortStrengths noncombat;e=world_air_port_combat(w,d,t,r,attacker,defender,offense,sensor.defense,noncombat);if(e!=LogicError::ok)return e;
 DefenseAbilitiesStrength late;e=world_defense_abilities(w,d,t,r,attacker,defender,noncombat.defense,late);if(e!=LogicError::ok)return e;
 const LandCombatStrengths result{noncombat.offense,late.defense,attack.offense_morale,defend.defense_morale,terrain.multiplier,multiplier,sensor.coverage};out=result;return LogicError::ok;
}
}
