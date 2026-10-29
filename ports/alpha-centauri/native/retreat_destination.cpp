/* SPDX-License-Identifier: GPL-3.0-or-later. See retreat_destination.h. */
#include "retreat_destination.h"
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
}
LogicError world_land_retreat_destination(const OriginalWorld&w,const UnitDefinitions&d,size_t defender,uint32_t incoming,bool network,const PodCoefficients&coeff,RetreatDestination&out){
 using detail::effects_range_overlap;
 if(!w.loaded()||incoming>7||!disjoint(w,&out,sizeof out)||effects_range_overlap(&d,sizeof d,&out,sizeof out)||effects_range_overlap(&coeff,sizeof coeff,&out,sizeof out))return LogicError::bad_state;
 // The current map importer normalizes this word with !=0; original retreat
 // and ZOC use bit 0. Hold noncanonical words until the shared map binding agrees.
 const auto* metadata=w.game().map.metadata;
 if(!metadata||metadata[28]>1||metadata[29]||metadata[30]||metadata[31])return LogicError::bad_state;
 const bool expansion=w.game().data_version>=11;
 if((expansion&&(d.weapon_count!=26||d.armor_count!=14||d.ability_count!=29))||(!expansion&&(d.weapon_count!=21||d.armor_count!=10||d.ability_count!=24)))return LogicError::bad_state;
 const auto*v=w.vehicle(defender);if(!v||v->unit_id<0||v->unit_id>=512||v->faction_id>7)return LogicError::bad_state;
 const auto*p=w.prototype((size_t)v->unit_id);if(!p||p->chassis_id>=9||p->weapon_id>=d.weapon_count||d.chassis[p->chassis_id].triad||d.chassis[p->chassis_id].missile)return LogicError::bad_state;
 // Original weapon offense is a signed byte; do not infer its sign from
 // a wider caller-owned rule value outside that wire domain.
 if(d.weapons[p->weapon_id].offense < -128||d.weapons[p->weapon_id].offense>127)return LogicError::bad_state;
 const auto&m=w.game().map.info;const MapGeometry g{(int32_t)m.width,(int32_t)m.height,m.flat};size_t origin;auto e=movement_tile_index(g,v->x,v->y,origin);if(e!=LogicError::ok)return e;
 uint32_t ignored;e=world_zone_of_control(w,v->x,v->y,v->faction_id,network,false,ignored);if(e!=LogicError::ok)return e;
 const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};RetreatDestination r{-1,0,0,0};uint32_t best=0;
 const bool native=v->unit_id<64&&(d.weapons[p->weapon_id].offense<0||(expansion&&v->unit_id==15));
 for(unsigned dir=0;dir<8;++dir){
  int32_t x=v->x+dx[dir],y=v->y+dy[dir];if(!m.flat){if(x<0)x+=g.width;else if(x>=g.width)x-=g.width;}
  if(x<0||y<0||x>=g.width||y>=g.height)continue;
  uint32_t zoc;e=world_zone_of_control(w,x,y,v->faction_id,network,true,zoc);if(e!=LogicError::ok)return e;if(zoc)continue;
  size_t ti;e=movement_tile_index(g,x,y,ti);if(e!=LogicError::ok)return e;const auto*t=w.tile(ti);if((t->climate&0xe0u)<0x60u)continue;
  uint32_t pod;e=tile_supply_pod(m,*t,x,y,(uint32_t)w.game().preferences.GameRules,coeff,pod);if(e!=LogicError::ok)return e;if(pod)continue;
  if((t->items&32u)&&(t->climate&0xe0u)>=0x40u&&!native){bool pholus=false;e=world_has_project(w,16,v->faction_id,pholus);if(e!=LogicError::ok)return e;if(!pholus)continue;}
  const uint32_t owner=t->val2&15u;if((t->items&3u)&&owner<8&&owner!=v->faction_id){const auto*f=w.faction(owner);if(!f||!(f->diplo_status[v->faction_id]&1u))continue;}
  r.eligible_mask|=1u<<dir;int delta=(int)dir-(int)(incoming^4u);if(delta<0)delta=-delta;uint32_t score=(uint32_t)delta&7u;if(score>4)score=8-score;
  if(score>2&&score>best){best=score;r.direction=(int32_t)dir;r.x=x;r.y=y;}
 }
 out=r;return LogicError::ok;
}
}
