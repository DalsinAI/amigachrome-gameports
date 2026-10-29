/* SPDX-License-Identifier: GPL-3.0-or-later. See combat_prototype.h. */
#include "combat_prototype.h"
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
}
LogicError world_prototype_combat_ratings(const OriginalWorld&w,const UnitDefinitions&d,const BasicRules&r,size_t id,size_t other,bool bombard,PrototypeCombatRatings&out){
 if(!w.loaded()||!disjoint(w,&out,sizeof out))return LogicError::bad_state;
 const auto*v=w.vehicle(id);const auto*o=w.vehicle(other);if(!v||!o||v->unit_id<0||o->unit_id<0)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);const auto*op=w.prototype((size_t)o->unit_id);if(!p||!op)return LogicError::bad_state;
 return prototype_combat_ratings(w.game().data_version<11?Variant::base:Variant::alien_crossfire,d,r,*p,(size_t)v->unit_id,*op,(size_t)o->unit_id,bombard,out);
}
LogicError world_combat_psi_factor(const OriginalWorld&w,size_t id,int32_t strength,bool attacking,int32_t&out){
 if(!w.loaded()||!disjoint(w,&out,sizeof out))return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v||v->faction_id>7||v->unit_id<0||v->unit_id>=512)return LogicError::bad_state;
 const auto*f=w.faction_definition(v->faction_id);if(!f)return LogicError::bad_state;
 bool project=false;const auto e=world_has_project(w,attacking?19:9,v->faction_id,project);if(e!=LogicError::ok)return e;
 return combat_psi_factor(w.game().data_version<11?Variant::base:Variant::alien_crossfire,strength,f->rule_psi,attacking,v->unit_id==19,!attacking&&project,attacking&&project,out);
}
}
