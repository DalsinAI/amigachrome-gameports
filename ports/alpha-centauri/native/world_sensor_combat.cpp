/* SPDX-License-Identifier: GPL-3.0-or-later. See sensor_combat.h. */
#include "sensor_combat.h"
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
int32_t absolute(int32_t x){return x<0?-x:x;}
LogicError nearest(const OriginalWorld&w,int32_t x,int32_t y,int32_t&out){
 int32_t best=9999,id=-1;const int32_t width=(int32_t)w.game().map.info.width;
 for(size_t i=0;i<w.game().base_count;++i){const auto*b=w.base(i);if(!b||b->x<0||b->x>=width||b->y<0||(uint32_t)b->y>=w.game().map.info.height)return LogicError::bad_state;
  int32_t dx=absolute(x-b->x);if(!w.game().map.info.flat&&dx>width/2)dx=width-dx;const int32_t dy=absolute(y-b->y);const int32_t high=dx>dy?dx:dy,low=dx<dy?dx:dy;const int32_t distance=high-((((dx+dy)/2)-low+1)/2);
  if(distance<=best){best=distance;id=(int32_t)i;}
 }
 out=id;return LogicError::ok;
}
LogicError sensor(const OriginalWorld&w,int32_t x,int32_t y,const OriginalMapTile&t,uint32_t&out){
 if(t.items&0x80000000u){out=1;return LogicError::ok;}
 if(w.game().data_version<11){out=0;return LogicError::ok;}
 int32_t id;auto e=nearest(w,x,y,id);if(e!=LogicError::ok)return e;out=0;if(id<0)return LogicError::ok;const auto*b=w.base((size_t)id);const int32_t width=(int32_t)w.game().map.info.width;int32_t dx=absolute(x-b->x);if(!w.game().map.info.flat&&dx>width/2)dx=width-dx;const int32_t dy=absolute(y-b->y);
 if((dx==0||dx==2)&&(dy==0||dy==2)&&(b->facilities_built[5]&2u))out=2;
 return LogicError::ok;
}
}
LogicError world_combat_sensor_coverage(const OriginalWorld&w,size_t defender,uint32_t&out){
 if(!w.loaded()||!disjoint(w,&out,sizeof out))return LogicError::bad_state;
 const auto*v=w.vehicle(defender);if(!v||v->faction_id>7)return LogicError::bad_state;const auto&info=w.game().map.info;const MapGeometry g{(int32_t)info.width,(int32_t)info.height,info.flat};size_t index;auto e=movement_tile_index(g,v->x,v->y,index);if(e!=LogicError::ok)return e;const auto*tile=w.tile(index);if(!tile)return LogicError::bad_state;
 if(!v->faction_id||(tile->climate&0xe0u)<0x60u){out=0;return LogicError::ok;}const auto*f=w.faction(v->faction_id);if(!f)return LogicError::bad_state;
 // Original RadiusOffsetX/Y first25 entries; includes the centre.
 static const int32_t dx[25]={0,1,2,1,0,-1,-2,-1,0,2,2,-2,-2,1,3,3,1,-1,-3,-3,-1,4,-4,0,0};
 static const int32_t dy[25]={0,-1,0,1,2,1,0,-1,-2,-2,2,2,-2,-3,-1,1,3,3,1,-1,-3,0,0,4,-4};
 uint32_t coverage=0;
 for(unsigned i=0;i<25;++i){int32_t x=v->x+dx[i],y=v->y+dy[i];if(!info.flat){if(x<0)x+=(int32_t)info.width;else if((uint32_t)x>=info.width)x-=(int32_t)info.width;}if(x<0||y<0||(uint32_t)x>=info.width||(uint32_t)y>=info.height)continue;
  size_t at;e=movement_tile_index(g,x,y,at);if(e!=LogicError::ok)return e;const auto*t=w.tile(at);if(!t)return LogicError::bad_state;uint32_t status;e=sensor(w,x,y,*t,status);if(e!=LogicError::ok)return e;if(!status)continue;bool valid=false;
  if((t->climate&0xe0u)<0x60u){int32_t id;e=nearest(w,x,y,id);if(e!=LogicError::ok)return e;if(id<0)return LogicError::bad_state;const auto*b=w.base((size_t)id);if(!b||b->faction_id>7)return LogicError::bad_state;valid=b->faction_id==v->faction_id;}
  else{const int32_t owner=t->owner;if(owner>7)return LogicError::bad_state;int32_t territory=owner;if(owner<1)territory=-1;else if(owner!=v->faction_id&&!((uint32_t)w.game().preferences.GameRules&0x80u)&&(f->diplo_status[owner]&0x08000008u)!=0x08000008u)territory=-1;valid=territory<0||territory==v->faction_id;}
  if(valid){coverage|=1u;if(status>1)coverage|=2u;}
 }
 out=coverage;return LogicError::ok;
}
LogicError world_sensor_combat_adjustment(const OriginalWorld&w,const BasicRules&r,size_t defender,int32_t defense,SensorCombatAdjustment&out){
 if(!w.loaded()||!disjoint(w,&out,sizeof out)||detail::effects_range_overlap(&r,sizeof r,&out,sizeof out))return LogicError::bad_state;
 uint32_t coverage;const auto e=world_combat_sensor_coverage(w,defender,coverage);if(e!=LogicError::ok)return e;return sensor_combat_adjustment(w.game().data_version<11?Variant::base:Variant::alien_crossfire,defense,coverage,r.combat_pct_def_range_sensor,out);
}
}
