/* SPDX-License-Identifier: GPL-3.0-or-later. See retreat_init.h. */
#include "retreat_init.h"
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
LogicError world_land_retreat_initialization(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t attacker,size_t defender,uint32_t flags,RetreatInitResult&out){
 using detail::effects_range_overlap;
 if(!w.loaded()||t.count!=89||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*a=w.vehicle(attacker);const auto*b=w.vehicle(defender);if(!a||!b||a->unit_id<0||b->unit_id<0||a->faction_id>7||b->faction_id>7)return LogicError::bad_state;
 const auto*ap=w.prototype((size_t)a->unit_id);const auto*bp=w.prototype((size_t)b->unit_id);if(!ap||!bp||ap->chassis_id>=9||bp->chassis_id>=9||bp->weapon_id>=d.weapon_count)return LogicError::bad_state;
 if(d.chassis[ap->chassis_id].triad||d.chassis[bp->chassis_id].triad||d.chassis[ap->chassis_id].missile||d.chassis[bp->chassis_id].missile)return LogicError::bad_state;
 uint32_t as,bs;auto e=world_vehicle_speed(w,d,t,attacker,r.move_rate_roads,true,as);if(e!=LogicError::ok)return e;e=world_vehicle_speed(w,d,t,defender,r.move_rate_roads,true,bs);if(e!=LogicError::ok)return e;
 bool jammer=false,diss=false;e=combat_ability(w,d,t,(size_t)a->unit_id,0x200u,jammer);if(e!=LogicError::ok)return e;
 if(w.game().data_version>=11&&jammer){e=combat_ability(w,d,t,(size_t)b->unit_id,0x2000000u,diss);if(e!=LogicError::ok)return e;}
 const MapGeometry g{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t tile;e=movement_tile_index(g,b->x,b->y,tile);if(e!=LogicError::ok)return e;const auto*mt=w.tile(tile);if(!mt)return LogicError::bad_state;
 // Follow actual serialized links in both directions, bounded against cycles.
 // Refuse broken same-tile stacks instead of quietly counting coordinates.
 size_t root=defender,steps=0;while(w.vehicle(root)->prev_veh_id_stack>=0){if(++steps>=w.game().vehicle_count)return LogicError::bad_state;const auto*v=w.vehicle(root);const size_t prev=(size_t)v->prev_veh_id_stack;const auto*p=w.vehicle(prev);if(!p||p->next_veh_id_stack!=(int32_t)root||p->x!=b->x||p->y!=b->y)return LogicError::bad_state;root=prev;}
 size_t at=root;uint32_t count=0;for(;;){const auto*v=w.vehicle(at);if(!v||v->x!=b->x||v->y!=b->y||++count>w.game().vehicle_count)return LogicError::bad_state;const int next=v->next_veh_id_stack;if(next<0)break;const auto*n=w.vehicle((size_t)next);if(!n||n->prev_veh_id_stack!=(int32_t)at)return LogicError::bad_state;at=(size_t)next;}
 uint32_t coordinate_count=0;for(size_t i=0;i<w.game().vehicle_count;++i){const auto*v=w.vehicle(i);if(v->x==b->x&&v->y==b->y)++coordinate_count;}if(count!=coordinate_count||!(mt->items&2u))return LogicError::bad_state;
 const RetreatInitFacts c{w.game().data_version>=11,as,bs,flags,d.weapons[bp->weapon_id].offense,0,0,b->state,b->order,mt->items,count,jammer,diss,(mt->items&1u)!=0,{(uint32_t)(uint8_t)bp->plan,(uint32_t)bp->reactor_id,b->damage_taken}};
 return retreat_initialization(c,out);
}
}
