/* SPDX-License-Identifier: GPL-3.0-or-later. See fungus_combat.h. */
#include "fungus_combat.h"
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

}
LogicError world_fungus_combat_adjustment(const OriginalWorld&w,const UnitDefinitions&d,size_t attacker,size_t defender,int32_t offense,FungusCombatStrength&out){
 using detail::effects_range_overlap;const bool expansion=w.game().data_version>=11;
 if(!w.loaded()||d.weapon_count!=(expansion?26u:21u)||d.armor_count!=(expansion?14u:10u)||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp)return LogicError::bad_state;
 uint32_t ignored;auto e=saved_prototype_speed(d,*ap,(size_t)a->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;e=saved_prototype_speed(d,*bp,(size_t)b->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;
 const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t index;e=movement_tile_index(g,b->x,b->y,index);if(e!=LogicError::ok)return e;const auto*tile=w.tile(index);if(!tile)return LogicError::bad_state;
 int32_t pholus=-1;const int32_t loc=w.game().preferences.SecretProjects[16];if(loc>=0){const auto*base=w.base((size_t)loc);if(!base||base->faction_id>7)return LogicError::bad_state;pholus=base->faction_id;}
 const FungusCombatContext c{offense,a->unit_id,d.weapons[ap->weapon_id].offense,d.weapons[bp->weapon_id].offense,tile->climate,tile->items,a->faction_id,pholus,(uint32_t)ap->plan};return fungus_combat_adjustment(c,out);
}
}
