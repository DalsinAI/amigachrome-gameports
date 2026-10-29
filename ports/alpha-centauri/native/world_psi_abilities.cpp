/* SPDX-License-Identifier: GPL-3.0-or-later. See world_psi_abilities.h. */
#include "world_psi_abilities.h"
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
LogicError world_psi_abilities(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,int32_t offense,int32_t defense,PsiAbilitiesStrengths&out){
 using detail::effects_range_overlap;
 const bool expansion=w.game().data_version>=11;
 if(!w.loaded()||t.count!=89||d.weapon_count!=(expansion?26u:21u)||d.armor_count!=(expansion?14u:10u)||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp||ap->plan==11)return LogicError::bad_state;
 uint32_t ignored;auto e=saved_prototype_speed(d,*ap,(size_t)a->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;e=saved_prototype_speed(d,*bp,(size_t)b->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;
 const int32_t missile=d.chassis[ap->chassis_id].missile;if(missile<0||missile>1)return LogicError::bad_state;
 PsiAbilitiesContext c{};c.variant=expansion?Variant::alien_crossfire:Variant::base;c.offense=offense;c.defense=defense;c.attacker_weapon=ap->weapon_id;c.defender_armor=bp->armor_id;c.attacker_plan=ap->plan;c.empath_percent=r.combat_pct_emp_song_atk_vs_psi;c.trance_percent=r.combat_pct_trance_def_vs_psi;
 if(!missile||(expansion&&(a->unit_id==15||b->unit_id==15))){if(d.weapons[ap->weapon_id].offense<0)c.psi_mask|=1;if(d.armor[bp->armor_id].defense<0)c.psi_mask|=2;}
 if(c.psi_mask&2u){if(expansion){e=combat_ability(w,d,t,(size_t)b->unit_id,0x2000000u,c.defender_dissociative);if(e!=LogicError::ok)return e;}if(!c.defender_dissociative){e=combat_ability(w,d,t,(size_t)a->unit_id,0x800u,c.attacker_empath);if(e!=LogicError::ok)return e;}}
 if(c.psi_mask&1u){if(expansion){e=combat_ability(w,d,t,(size_t)a->unit_id,0x2000000u,c.attacker_dissociative);if(e!=LogicError::ok)return e;}if(!c.attacker_dissociative){e=combat_ability(w,d,t,(size_t)b->unit_id,0x40000u,c.defender_trance);if(e!=LogicError::ok)return e;}}
 return psi_abilities(c,out);
}
}
