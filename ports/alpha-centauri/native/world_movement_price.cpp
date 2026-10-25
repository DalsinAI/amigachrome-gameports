/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_movement_price.h"
namespace alpha {
// Only Antigrav/Slow are queried by land pricing/prototype speed. Alien radar
// and Nethack algorithm bonuses cannot answer either of these queries.
static LogicError land_design_flags(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t id,uint32_t& flags){
 if(id<64 || (flags&0x800400u)==0x800400u)return LogicError::ok;
 const auto* f=w.faction_definition(id/64);if(!f || f->faction_bonus_count<0 || f->faction_bonus_count>8)return LogicError::bad_state;
 for(int i=0;i<f->faction_bonus_count;++i)if(f->faction_bonus_id[i]==15){
  const int a=f->faction_bonus_val1[i];if(a<0 || (uint32_t)a>=d.ability_count)return LogicError::bad_state;
  const uint32_t flag=1u<<a;if(!(flag&0x800400u) || (flags&flag))continue;
  bool owned;auto error=world_has_technology(w,tech,d.abilities[a].prerequisite,(int32_t)(id/64),owned);if(error!=LogicError::ok)return error;
  if(owned)flags|=flag;
 }
 return LogicError::ok;
}
LogicError world_movement_price(const OriginalWorld& w,const UnitDefinitions& d,const Rules& tech,size_t index,
 int32_t road,int32_t tx,int32_t ty,bool ignore_rough,uint32_t& output){
 if(!w.loaded() || !((w.game().data_version<11 && d.weapon_count==21) || (w.game().data_version>=11 && d.weapon_count==26)))return LogicError::bad_state;
 const auto* v=w.vehicle(index);if(!v || v->unit_id<0)return LogicError::bad_state;const auto* p=w.prototype((size_t)v->unit_id);if(!p)return LogicError::bad_state;
 uint32_t speed;auto error=saved_prototype_speed(d,*p,(size_t)v->unit_id,road,false,speed);if(error!=LogicError::ok)return error;
 OriginalPrototype effective=*p;
 if(d.chassis[p->chassis_id].triad==0){
  error=land_design_flags(w,d,tech,(size_t)v->unit_id,effective.ability_flags);if(error!=LogicError::ok)return error;
  error=saved_prototype_speed(d,effective,(size_t)v->unit_id,road,false,speed);if(error!=LogicError::ok)return error;
 }
 MapGeometry geometry{(int32_t)w.game().map.info.width,(int32_t)w.game().map.info.height,w.game().map.info.flat};size_t source,dest;int32_t dx;
 if(movement_tile_index(geometry,v->x,v->y,source)!=LogicError::ok || movement_tile_index(geometry,tx,ty,dest)!=LogicError::ok || movement_distance_x(geometry,v->x,tx,dx)!=LogicError::ok)return LogicError::bad_state;
 int32_t dy=v->y>ty?v->y-ty:ty-v->y;
 if(!((dx==1 && dy==1) || (dx==2 && dy==0) || (dx==0 && dy==2)))return LogicError::bad_state;
 const auto* from=w.tile(source);const auto* to=w.tile(dest);const auto* faction=w.faction(v->faction_id);if(!from || !to || !faction)return LogicError::bad_state;
 bool xeno;error=world_has_project(w,8,v->faction_id,xeno);if(error!=LogicError::ok)return error;
 const bool expansion=w.game().data_version>=11;
 MovementDesign design{(uint32_t)v->unit_id,effective.ability_flags,speed,p->chassis_id,d.chassis[p->chassis_id].triad,d.weapons[p->weapon_id].offense,p->plan,
  v->unit_id==9 || (expansion && v->unit_id==14),v->unit_id==8 || (expansion && v->unit_id==15)};
 MovementContext context{v->faction_id,faction->SE_planet,road,xeno,ignore_rough};
 MovementTile a{from->items,(uint8_t)(from->climate&0xe0u),(uint8_t)(from->val3&0xc0u)},b{to->items,(uint8_t)(to->climate&0xe0u),(uint8_t)(to->val3&0xc0u)};
 // Prototype speed affects only the land/fungus path, where Cloudbase is inert.
 return terrain_step_price(a,b,dx==1 && dy==1,design,context,output);
}
}
