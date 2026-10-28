/* SPDX-License-Identifier: GPL-3.0-or-later. See base_combat.h. */
#include "base_combat.h"
#include "world_unit_stats.h"
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
LogicError project(const OriginalWorld&w,size_t id,int32_t faction,bool&out){return world_has_project(w,id,faction,out);}
LogicError blink(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,size_t id,bool&out){
 const auto*p=w.prototype(id);if(!p)return LogicError::bad_state;if(p->ability_flags&0x20000u){out=true;return LogicError::ok;}
 if(w.game().data_version<11||id<64){out=false;return LogicError::ok;}const auto*f=w.faction_definition(id/64);if(!f||f->faction_bonus_count<0||f->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){const int a=f->faction_bonus_val1[i];if(a<0||(uint32_t)a>=d.ability_count)return LogicError::bad_state;if(a!=17)continue;bool known=false;const auto e=world_has_technology(w,t,d.abilities[a].prerequisite,(int32_t)(id/64),known);if(e!=LogicError::ok)return e;if(known){out=true;return LogicError::ok;}}
 out=false;return LogicError::ok;
}
}
LogicError world_base_combat_adjustment(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t defender,int32_t attacker,int32_t offense,int32_t defense,BaseCombatAdjustment&out){
 using detail::effects_range_overlap;
 if(!w.loaded()||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out)||attacker< -1||t.count!=89||d.weapon_count!=(w.game().data_version<11?21u:26u))return LogicError::bad_state;
 const auto*v=w.vehicle(defender);if(!v||v->faction_id>7)return LogicError::bad_state;const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t tile;auto e=movement_tile_index(g,v->x,v->y,tile);if(e!=LogicError::ok)return e;const auto*m=w.tile(tile);if(!m||!(m->items&1u))return LogicError::bad_state;
 const OriginalBase*b=nullptr;for(size_t i=0;i<w.game().base_count;++i){const auto*item=w.base(i);if(!item)return LogicError::bad_state;if(item->x==v->x&&item->y==v->y){b=item;break;}}if(!b||b->faction_id>7)return LogicError::bad_state;
 BaseCombatContext c{};c.offense=offense;c.defense=defense;c.attacker_present=attacker>=0;c.perimeter=(b->facilities_built[0]&16u)!=0;c.naval_yard=(b->facilities_built[3]&16u)!=0;c.aerospace=(b->facilities_built[3]&32u)!=0;c.tachyon=(b->facilities_built[0]&32u)!=0;c.base_percent=r.combat_pct_base_def;c.infantry_percent=r.combat_pct_infantry_vs_base;
 if(c.attacker_present){const auto*a=w.vehicle((size_t)attacker);if(!a||a->faction_id>7||a->unit_id<0||a->unit_id>=512)return LogicError::bad_state;const auto*p=w.prototype((size_t)a->unit_id);if(!p||p->chassis_id>=9||p->weapon_id>=d.weapon_count)return LogicError::bad_state;c.attacker_prototype=a->unit_id;c.attacker_weapon=d.weapons[p->weapon_id].offense;c.attacker_triad=d.chassis[p->chassis_id].triad;c.attacker_speed=d.chassis[p->chassis_id].speed;c.attacker_faction=a->faction_id;
  // Citizens uses the defender actor faction; Maritime/Cloudbase use the
  // physical base owner in the original. Preserve mixed-owner inspections.
  if(c.attacker_triad==0&&!c.perimeter){e=project(w,5,v->faction_id,c.citizens);if(e!=LogicError::ok)return e;}
  if(c.attacker_triad==1&&!c.naval_yard){e=project(w,10,b->faction_id,c.maritime);if(e!=LogicError::ok)return e;}
  if(c.attacker_triad==2&&!c.aerospace&&w.game().data_version>=11){e=project(w,35,b->faction_id,c.cloudbase);if(e!=LogicError::ok)return e;}
  e=blink(w,d,t,(size_t)a->unit_id,c.blink);if(e!=LogicError::ok)return e;
 }
 return base_combat_adjustment(w.game().data_version<11?Variant::base:Variant::alien_crossfire,c,out);
}
}
