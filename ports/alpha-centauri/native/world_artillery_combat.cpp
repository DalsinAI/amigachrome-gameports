/* SPDX-License-Identifier: GPL-3.0-or-later. See world_artillery_combat.h. */
#include "world_artillery_combat.h"
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
LogicError world_artillery_combat(const OriginalWorld&w,const UnitDefinitions&d,const BasicRules&r,size_t attacker,size_t defender,uint32_t battle_flags,int32_t offense,int32_t defense,ArtilleryStrengths&out){
 using detail::effects_range_overlap;
 const bool expansion=w.game().data_version>=11;
 if(!w.loaded()||d.weapon_count!=(expansion?26u:21u)||d.armor_count!=(expansion?14u:10u)||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||a->unit_id>=512||b->unit_id<0||b->unit_id>=512||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp)return LogicError::bad_state;
 uint32_t ignored;auto e=saved_prototype_speed(d,*ap,(size_t)a->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;e=saved_prototype_speed(d,*bp,(size_t)b->unit_id,1,false,ignored);if(e!=LogicError::ok)return e;
 const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t ai,bi;e=movement_tile_index(g,a->x,a->y,ai);if(e!=LogicError::ok)return e;e=movement_tile_index(g,b->x,b->y,bi);if(e!=LogicError::ok)return e;const auto*at=w.tile(ai);const auto*bt=w.tile(bi);if(!at||!bt)return LogicError::bad_state;
 ArtilleryContext c{offense,defense,(uint32_t)d.chassis[ap->chassis_id].triad,(uint32_t)d.chassis[bp->chassis_id].triad,at->climate,bt->climate,battle_flags,r.combat_pct_land_gun_vs_ship_art,r.combat_pct_art_bonus_lvl_alt};
 return artillery_combat(c,out);
}
}
