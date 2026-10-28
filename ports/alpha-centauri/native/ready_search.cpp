/* SPDX-License-Identifier: GPL-3.0-or-later. Original offline ready search. */
#include "ready_search.h"
#include "technology_effects_ranges.h"
#include <limits.h>
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

static LogicError ready_search_internal(const OriginalWorld&w,const UnitDefinitions&units,const Rules&tech,
 int32_t human,int32_t road,const ReadySearchFacts&facts,ReadySearchResult&out,int32_t defer_target){
 if(!w.loaded() || tech.count!=89 || road<1 || road>100 ||
    !((w.game().data_version<11 && units.weapon_count==21 && units.armor_count==10 && units.ability_count==24) ||
      (w.game().data_version>=11 && units.weapon_count==26 && units.armor_count==14 && units.ability_count==29)) || human<1 || human>7 || w.game().vehicle_count>2048 ||
    !disjoint(w,&out,sizeof out) || detail::effects_range_overlap(&out,sizeof out,&units,sizeof units) ||
    detail::effects_range_overlap(&out,sizeof out,&tech,sizeof tech) ||
    detail::effects_range_overlap(&out,sizeof out,&facts,sizeof facts))return LogicError::bad_state;
 const auto&map=w.game().map.info;
 if(map.width>INT32_MAX || map.height>INT32_MAX || facts.reference< -1 || facts.current< -1 ||
    (facts.reference>=0&&(uint32_t)facts.reference>=w.game().vehicle_count) ||
    (facts.current>=0&&(uint32_t)facts.current>=w.game().vehicle_count))return LogicError::bad_state;
 int32_t x=facts.cursor_x,y=facts.cursor_y;
 const auto*reference=facts.reference>=0?w.vehicle((size_t)facts.reference):nullptr;
 const bool local_reference=reference&&reference->faction_id==human;
 if(local_reference){x=reference->x;y=reference->y;}
 if(x<0 || y<0 || (uint32_t)x>=map.width || (uint32_t)y>=map.height || ((x+y)&1))return LogicError::bad_state;
 ReadySearchResult next{};next.selected=-1;next.distance=9999;
 for(unsigned pass=0;pass<3;++pass){
  next.passes=pass+1;
  for(size_t i=0;i<w.game().vehicle_count;++i){
   UnitReadiness r{};const auto e=world_unit_readiness(w,units,tech,i,human,road,r);if(e!=LogicError::ok)return e;
   const auto*v=w.vehicle(i);const bool deferred=((v->state&2u)||(int32_t)i==defer_target)&&!(next.clear_deferred[i/8]&(1u<<(i%8)));
   if(!r.ready || deferred)continue;
   int64_t dx=(int64_t)x-v->x,dy=(int64_t)y-v->y;if(dx<0)dx=-dx;if(dy<0)dy=-dy;
   if(!map.flat && dx>(int64_t)map.width/2)dx=(int64_t)map.width-dx;
   if(dx<0)dx=-dx;
   const int64_t high=dx>dy?dx:dy,low=dx<dy?dx:dy;
   const int64_t distance=high-(((dx+dy)/2-low+1)/2);
   if(distance<0 || distance>INT32_MAX)return LogicError::overflow;
   if(distance<next.distance || (distance==next.distance&&(int32_t)i==facts.current)){
    next.distance=(int32_t)distance;next.selected=(int32_t)i;
   }
  }
  if(next.selected>=0){out=next;return LogicError::ok;}
  for(size_t i=0;i<w.game().vehicle_count;++i)if(pass!=0 || (int32_t)i!=facts.current || !local_reference){
   if((w.vehicle(i)->state&2u)||(int32_t)i==defer_target)next.clear_deferred[i/8]|=(uint8_t)(1u<<(i%8));
  }
 }
 out=next;return LogicError::ok;
}
LogicError world_ready_search(const OriginalWorld&w,const UnitDefinitions&u,const Rules&t,int32_t h,int32_t r,const ReadySearchFacts&f,ReadySearchResult&out){
 return ready_search_internal(w,u,t,h,r,f,out,-1);
}
LogicError world_wait_ready_search(const OriginalWorld&w,const UnitDefinitions&u,const Rules&t,int32_t h,int32_t r,size_t id,int32_t x,int32_t y,ReadySearchResult&out){
 if(id>INT32_MAX)return LogicError::bad_state;
 const auto*v=w.vehicle(id);if(!v||v->faction_id!=h)return LogicError::bad_state;
 const ReadySearchFacts facts{(int32_t)id,x,y,(int32_t)id};
 return ready_search_internal(w,u,t,h,r,facts,out,(int32_t)id);
}
}
