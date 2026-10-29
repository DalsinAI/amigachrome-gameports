/* SPDX-License-Identifier: GPL-3.0-or-later. See world_air_port_combat.h. */
#include "world_air_port_combat.h"
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
LogicError physical_base(const OriginalWorld&w,const OriginalVehicle&v,int32_t&out){
 const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t at;if(movement_tile_index(g,v.x,v.y,at)!=LogicError::ok)return LogicError::bad_state;const auto*tile=w.tile(at);if(!tile)return LogicError::bad_state;
 if(!(tile->items&1u)){out=-1;return LogicError::ok;}
 for(size_t i=0;i<w.game().base_count;++i){const auto*b=w.base(i);if(!b)return LogicError::bad_state;if(b->x==v.x&&b->y==v.y){out=(int32_t)i;return LogicError::ok;}}
 return LogicError::bad_state;
}
}
LogicError world_air_port_combat(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,int32_t offense,int32_t defense,AirPortStrengths&out){
 using detail::effects_range_overlap;
 const bool expansion=w.game().data_version>=11;
 if(!w.loaded()||t.count!=89||d.weapon_count!=(expansion?26u:21u)||d.armor_count!=(expansion?14u:10u)||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp)return LogicError::bad_state;
 uint32_t ignored;auto e=saved_prototype_speed(d,*ap,(size_t)a->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;e=saved_prototype_speed(d,*bp,(size_t)b->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;
 if(d.weapons[ap->weapon_id].offense<0||d.armor[bp->armor_id].defense<0)return LogicError::bad_state;
 const auto&ac=d.chassis[ap->chassis_id];const auto&bc=d.chassis[bp->chassis_id];if(ac.missile<0||ac.missile>1||bc.missile<0||bc.missile>1)return LogicError::bad_state;
 int32_t base;e=physical_base(w,*b,base);if(e!=LogicError::ok)return e;
 AirPortContext c{};c.offense=offense;c.defense=defense;c.attacker_triad=(uint32_t)ac.triad;c.defender_triad=(uint32_t)bc.triad;c.attacker_missile=ac.missile!=0;c.defender_missile=bc.missile!=0;c.defender_weapon=d.weapons[bp->weapon_id].offense;c.defender_armor=d.armor[bp->armor_id].defense;c.attacker_faction=a->faction_id;c.defender_base=base>=0;
 c.ground_strike_penalty=r.combat_pen_pct_air_supr_vs_grnd;c.air_superiority_percent=r.combat_pct_air_supr_vs_air;c.noncombat_penalty=r.combat_pen_pct_non_cbt_def_vs_cbt;c.port_percent=r.combat_pct_bonus_vs_ship_port;
 if(c.attacker_triad==2){e=combat_ability(w,d,t,(size_t)a->unit_id,0x20u,c.attacker_superiority);if(e!=LogicError::ok)return e;if(c.defender_triad==2){e=combat_ability(w,d,t,(size_t)b->unit_id,0x20u,c.defender_superiority);if(e!=LogicError::ok)return e;}}
 return air_port_combat(c,out);
}
}
