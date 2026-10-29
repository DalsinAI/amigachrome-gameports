/* SPDX-License-Identifier: GPL-3.0-or-later. See world_combat_factions.h. */
#include "world_combat_factions.h"
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
LogicError combat_ability(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,size_t id,uint32_t mask,bool&out){
 const auto*p=w.prototype(id);if(!p)return LogicError::bad_state;
 if(p->ability_flags&mask){out=true;return LogicError::ok;}
 if(w.game().data_version<11||id<64){out=false;return LogicError::ok;}
 const auto*f=w.faction_definition(id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){const int a=f->faction_bonus_val1[i];if(a<0||(uint32_t)a>=d.ability_count)return LogicError::bad_state;if((1u<<a)!=mask)continue;bool achieved;const auto e=world_has_technology(w,t,d.abilities[a].prerequisite,(int32_t)(id/64),achieved);if(e!=LogicError::ok)return e;if(achieved){out=true;return LogicError::ok;}}
 out=false;return LogicError::ok;
}
}
LogicError world_combat_faction_modifiers(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,int32_t offense,int32_t defense,CombatFactionStrengths&out){
 using detail::effects_range_overlap;
 const bool expansion=w.game().data_version>=11;
 if(!w.loaded()||t.count!=89||d.weapon_count!=(expansion?26u:21u)||d.armor_count!=(expansion?14u:10u)||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp||ap->plan==11)return LogicError::bad_state;
 uint32_t ignored;auto e=saved_prototype_speed(d,*ap,(size_t)a->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;e=saved_prototype_speed(d,*bp,(size_t)b->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;
 const int32_t missile=d.chassis[ap->chassis_id].missile;if(missile<0||missile>1)return LogicError::bad_state;
 CombatFactionContext c{};c.variant=expansion?Variant::alien_crossfire:Variant::base;c.offense=offense;c.defense=defense;c.attacker_plan=ap->plan;c.attacker_faction=a->faction_id;
 if(!missile||(expansion&&(a->unit_id==15||b->unit_id==15))){if(d.weapons[ap->weapon_id].offense<0)c.psi_mask|=1;if(d.armor[bp->armor_id].defense<0)c.psi_mask|=2;}
 const auto*af=w.faction_definition(a->faction_id);const auto*bf=w.faction_definition(b->faction_id);const auto*state=w.faction(a->faction_id);if(!af||!bf||!state)return LogicError::bad_state;
 c.made_airdrop=(a->state&32u)!=0;c.fanatic=(af->rule_flags&0x400)!=0;c.active_planet=state->SE_planet;c.airdrop_penalty=r.combat_pen_pct_atk_airdrop;c.fanatic_bonus=r.combat_pct_fanatic_atk_bonus;c.planet_percent=r.combat_pct_psi_atk_bonus_planet;
 if(c.made_airdrop&&c.airdrop_penalty){e=combat_ability(w,d,t,(size_t)a->unit_id,16u,c.drop_pods);if(e!=LogicError::ok)return e;}
 if(expansion){if(af->faction_bonus_count<0||af->faction_bonus_count>8||bf->faction_bonus_count<0||bf->faction_bonus_count>8)return LogicError::bad_state;for(int i=0;i<af->faction_bonus_count;++i)if(af->faction_bonus_id[i]==18)c.offense_factors[c.offense_count++]=af->faction_bonus_val1[i];for(int i=0;i<bf->faction_bonus_count;++i)if(bf->faction_bonus_id[i]==17)c.defense_factors[c.defense_count++]=bf->faction_bonus_val1[i];}
 return combat_faction_modifiers(c,out);
}
}
