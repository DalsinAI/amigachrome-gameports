/* SPDX-License-Identifier: GPL-3.0-or-later. See terrain_defense.h. */
#include "terrain_defense.h"
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
bool components(const OriginalWorld&w,const UnitDefinitions&d,const OriginalVehicle*v,int32_t&proto,int32_t&weapon,int32_t&triad){
 if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7)return false;
 const auto*p=w.prototype((size_t)v->unit_id);if(!p||p->chassis_id>=9||p->weapon_id>=d.weapon_count)return false;
 proto=v->unit_id;weapon=d.weapons[p->weapon_id].offense;triad=d.chassis[p->chassis_id].triad;return true;
}
}
LogicError world_terrain_defense(const OriginalWorld&w,const UnitDefinitions&d,size_t defender,int32_t attacker,TerrainDefense&out){
 if(!w.loaded()||!disjoint(w,&out,sizeof out)||detail::effects_range_overlap(&d,sizeof d,&out,sizeof out)||attacker< -1||d.weapon_count!=(w.game().data_version<11?21u:26u))return LogicError::bad_state;
 const auto*v=w.vehicle(defender);TerrainDefenseContext c{};if(!components(w,d,v,c.defender_prototype,c.defender_weapon,c.defender_triad))return LogicError::bad_state;c.defender_faction=v->faction_id;
 const MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t index;auto e=movement_tile_index(geometry,v->x,v->y,index);if(e!=LogicError::ok)return e;const auto*t=w.tile(index);if(!t)return LogicError::bad_state;
 c.altitude_byte=t->climate;c.roughness_byte=t->val3;c.items=t->items;c.owner_nibble=t->val2&15u;c.pholus_owner=-1;
 const int32_t location=w.game().preferences.SecretProjects[16];if(location>=0){const auto*b=w.base((size_t)location);if(!b||b->faction_id>7)return LogicError::bad_state;c.pholus_owner=b->faction_id;}
 c.attacker_present=attacker>=0;if(c.attacker_present){const auto*a=w.vehicle((size_t)attacker);if(!components(w,d,a,c.attacker_prototype,c.attacker_weapon,c.attacker_triad))return LogicError::bad_state;c.attacker_faction=a->faction_id;}
 return terrain_defense(c,out);
}
}
