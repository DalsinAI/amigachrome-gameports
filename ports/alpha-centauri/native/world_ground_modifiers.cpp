/* SPDX-License-Identifier: GPL-3.0-or-later. See ground_modifiers.h. */
#include "ground_modifiers.h"
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
LogicError speed(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,const OriginalVehicle*v,int32_t&out,bool&grav){
 if(!v||v->faction_id>7||v->unit_id<0||v->unit_id>=512)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);if(!p||p->chassis_id>=9||d.chassis[p->chassis_id].triad!=0)return LogicError::bad_state;OriginalPrototype effective=*p;auto e=world_speed_ability_flags(w,d,t,(size_t)v->unit_id,effective.ability_flags);if(e!=LogicError::ok)return e;uint32_t value;e=saved_prototype_speed(d,effective,(size_t)v->unit_id,r.move_rate_roads,false,value);if(e!=LogicError::ok)return e;out=(int32_t)value;grav=(effective.ability_flags&0x400u)!=0;return LogicError::ok;
}
}
LogicError world_ground_combat_modifiers(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,int32_t offense,int32_t defense,GroundModifierStrengths&out){
 using detail::effects_range_overlap;
 if(!w.loaded()||t.count!=89||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 GroundModifierContext c{};c.offense=offense;c.defense=defense;c.road_rate=r.move_rate_roads;const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);auto e=speed(w,d,t,r,a,c.attacker_speed,c.attacker_antigrav);if(e!=LogicError::ok)return e;e=speed(w,d,t,r,b,c.defender_speed,c.defender_antigrav);if(e!=LogicError::ok)return e;
 const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t at,bt;e=movement_tile_index(g,a->x,a->y,at);if(e!=LogicError::ok)return e;e=movement_tile_index(g,b->x,b->y,bt);if(e!=LogicError::ok)return e;const auto*am=w.tile(at);const auto*bm=w.tile(bt);if(!am||!bm)return LogicError::bad_state;c.attacker_altitude=am->climate;c.defender_altitude=bm->climate;c.roughness=bm->val3;c.physical_base=(bm->items&1u)!=0;
 if(c.physical_base){bool found=false;for(size_t i=0;i<w.game().base_count;++i){const auto*base=w.base(i);if(!base)return LogicError::bad_state;if(base->x==b->x&&base->y==b->y){found=true;break;}}if(!found)return LogicError::bad_state;}
 TerrainDefense terrain;e=world_terrain_defense(w,d,defender,(int32_t)attacker,terrain);if(e!=LogicError::ok)return e;c.terrain=terrain.multiplier;c.mobile_percent=r.combat_pct_mobile_open_ground;c.rough_percent=r.combat_pct_def_vs_mobile_rough;c.road_percent=r.combat_pct_atk_road;c.uphill_percent=r.combat_pen_pct_atk_lwr_elev;c.downhill_percent=r.combat_pct_atk_higher_elev;
 return ground_combat_modifiers(c,out);
}
}
