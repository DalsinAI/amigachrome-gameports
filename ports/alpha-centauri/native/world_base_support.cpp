/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker default support policy: dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * MIT, Copyright (c) Thinker Mod authors; see THINKER-LICENSE.txt.
 */
#include "world_base_support.h"
#include <stdlib.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool alias(const OriginalWorld&w,const UnitDefinitions&d,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&d,sizeof d))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
SupportError world_base_support_policy(const OriginalWorld&w,const UnitDefinitions&d,
 size_t id,SupportResult&output,uint32_t*states,size_t capacity){
 if(!w.loaded())return SupportError::bad_state;
 const auto&g=w.game();const size_t count=g.vehicle_count;
 if(count>SIZE_MAX/sizeof(uint32_t)||count>SIZE_MAX/sizeof(SupportUnit))return SupportError::overflow;
 if((count&&!states)||capacity<count)return SupportError::bad_buffer;
 if(alias(w,d,&output,sizeof output)||alias(w,d,states,count*sizeof(uint32_t))||overlap(states,count*sizeof(uint32_t),&output,sizeof output))return SupportError::bad_buffer;
 const auto*b=w.base(id);if(!b||b->faction_id<1||b->faction_id>7||b->pop_size<0)return SupportError::bad_state;
 if(g.data_version>=11||d.weapon_count!=21||!(g.preferences.FactionStatus[0]&(1u<<(unsigned)b->faction_id)))return SupportError::unsupported;
 const auto*f=w.faction((size_t)b->faction_id);if(!f)return SupportError::bad_state;
 SupportFacts facts{f->SE_support_pending,f->SE_police_pending,b->pop_size};
 auto*units=count?(SupportUnit*)malloc(count*sizeof(SupportUnit)):nullptr;
 if(count&&!units)return SupportError::out_of_memory;
 SupportError error=SupportError::ok;
 const auto&m=g.map.info;
 for(size_t i=0;i<count;++i){
  const auto*v=w.vehicle(i);if(!v||v->faction_id>7||v->unit_id<0||v->unit_id>=512||v->home_base_id< -1||v->home_base_id>=(int32_t)g.base_count){error=SupportError::bad_state;break;}
  auto&u=units[i];u={v->state,0,0,0,v->faction_id==b->faction_id&&v->home_base_id==(int32_t)id,false,false,false};
  if(!u.homed_here)continue;
  const auto*p=w.prototype((size_t)v->unit_id);
  if(!p||p->chassis_id>=9||p->weapon_id>=d.weapon_count||p->plan<0||p->plan>14||d.chassis[p->chassis_id].triad<0||d.chassis[p->chassis_id].triad>2||v->x<0||v->y<0||(uint32_t)v->x>=m.width||(uint32_t)v->y>=m.height||((v->x+v->y)&1)){error=SupportError::bad_state;break;}
  const auto*t=w.tile(((size_t)v->y*m.width+(size_t)v->x)/2);
  if(!t||t->owner< -1||t->owner>7){error=SupportError::bad_state;break;}
  u.plan=p->plan;u.offense=d.weapons[p->weapon_id].offense;u.triad=d.chassis[p->chassis_id].triad;u.clean_reactor=(p->ability_flags&0x10000u)!=0;
  u.native_on_fungus=v->unit_id<64&&u.offense<0&&(t->items&0x20u)&&(t->climate&0xe0u)>=0x40u;
  // whose_territory may hide another faction's identity, but cannot turn it
  // into this faction. Equality is therefore answerable directly from owner.
  const bool base=(t->items&1u)&&(t->val2&15u)<8;
  u.outside_own_territory_and_base=!base&&t->owner!=b->faction_id;
 }
 if(error==SupportError::ok)error=base_support_policy(facts,units,count,output,states,capacity);
 free(units);return error;
}
}
