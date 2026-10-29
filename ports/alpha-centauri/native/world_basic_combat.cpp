/* SPDX-License-Identifier: GPL-3.0-or-later. See basic_combat.h. */
#include "basic_combat.h"
#include "native_combat_morale.h"
#include "world_unit_stats.h"
#include "movement.h"
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
LogicError world_basic_combat_strengths(const OriginalWorld&w,const UnitDefinitions&d,const Rules&t,const BasicRules&r,size_t actor,size_t opposing,bool psi,bool bombard,bool mode,BasicCombatStrengths&out){
 if(!w.loaded()||t.count!=89||!disjoint(w,&out,sizeof out)||detail::effects_range_overlap(&d,sizeof d,&out,sizeof out)||detail::effects_range_overlap(&t,sizeof t,&out,sizeof out)||detail::effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 const auto*v=w.vehicle(actor);const auto*o=w.vehicle(opposing);if(!v||!o||v->unit_id<0||o->unit_id<0||v->faction_id>7||o->faction_id>7)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);const auto*op=w.prototype((size_t)o->unit_id);if(!p||!op)return LogicError::bad_state;
 PrototypeCombatRatings ratings;auto e=world_prototype_combat_ratings(w,d,r,actor,opposing,bombard,ratings);if(e!=LogicError::ok)return e;
 uint32_t morale=0;e=v->faction_id?world_effective_morale(w,d,t,actor,true,morale):world_native_combat_morale(w,actor,o->faction_id,morale);if(e!=LogicError::ok)return e;
 const auto*f=w.faction(v->faction_id);if(!f)return LogicError::bad_state;
 int32_t base=-1;e=physical_base(w,*v,base);if(e!=LogicError::ok)return e;bool creche=false,brood=false;
 if(base>=0){const auto*b=w.base((size_t)base);creche=(b->facilities_built[0]&4u)!=0;brood=(b->facilities_built[4]&8u)!=0;}
 const int32_t off=d.weapons[p->weapon_id].offense,armor=d.armor[p->armor_id].defense;
 bool disso=false,sop=false;
 if(w.game().data_version>=11&&o->faction_id&&(v->unit_id>=64||(off>=0&&v->unit_id!=15))){e=combat_ability(w,d,t,(size_t)v->unit_id,0x2000000u,disso);if(e!=LogicError::ok)return e;if(!disso){e=combat_ability(w,d,t,(size_t)o->unit_id,0x1000000u,sop);if(e!=LogicError::ok)return e;}}
 const bool fixed=p->plan==12||(p->plan==11&&armor==1&&op->plan!=11);
 int32_t offense=ratings.offense,defense=ratings.defense;if(psi){e=world_combat_psi_factor(w,actor,offense,true,offense);if(e!=LogicError::ok)return e;if(!fixed){e=world_combat_psi_factor(w,actor,defense,false,defense);if(e!=LogicError::ok)return e;}}
 const BasicCombatContext c{v->unit_id,off,armor,p->plan,op->plan,(int32_t)morale,f->SE_morale,f->SE_morale_pending,base>=0,creche,brood,o->faction_id==0,disso,sop,mode,offense,defense};
 return basic_combat_strengths(w.game().data_version<11?Variant::base:Variant::alien_crossfire,c,out);
}
}
