/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in corresponding header. */
#include "world_technology_effects.h"
#include "world_unit_stats.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool aliases(const OriginalWorld&w,const Rules&r,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&r,sizeof r))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||
  overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||
  overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||
  overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||
  overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||
  overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||
  overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
bool add(int32_t& value,int32_t amount){
 const int64_t next=(int64_t)value+amount;
 if(next<(-2147483647LL-1)||next>2147483647LL)return false;
 value=(int32_t)next;return true;
}
}
TechnologyEffectsError world_technology_effects(const OriginalWorld&w,const Rules&r,
 int32_t faction,TechnologyEffects&output){
 if(aliases(w,r,&output,sizeof output))return TechnologyEffectsError::bad_buffer;
 if(!w.loaded()||r.count!=technology_count||faction<0||faction>=8)return TechnologyEffectsError::bad_state;
 const auto*f=w.faction((size_t)faction);const auto*d=w.faction_definition((size_t)faction);
 if(!f||!d||d->faction_bonus_count<0||d->faction_bonus_count>8)return TechnologyEffectsError::bad_state;
 for(size_t i=0;i<technology_count;++i)for(unsigned k=0;k<2;++k)
  if(r.technologies[i].prerequisites[k]<-2||r.technologies[i].prerequisites[k]>=(int32_t)technology_count)
   return TechnologyEffectsError::bad_state;
 TechnologyEffects next{};next.commerce=d->rule_commerce;
 for(int32_t i=0;i<(int32_t)technology_count;++i){
  bool owned=false;if(world_has_technology(w,r,i,faction,owned)!=LogicError::ok)return TechnologyEffectsError::bad_state;
  if(!owned)continue;
  const uint32_t flags=r.technologies[i].flags;
  if((flags&0x100u)&&!add(next.fungus[0],1))return TechnologyEffectsError::overflow;
  if((flags&0x80u)&&!add(next.fungus[1],1))return TechnologyEffectsError::overflow;
  if((flags&0x40u)&&!add(next.fungus[2],1))return TechnologyEffectsError::overflow;
  if((flags&4u)&&!add(next.commerce,1))return TechnologyEffectsError::overflow;
 }
 for(int32_t i=0;i<d->faction_bonus_count;++i){
  const int32_t id=d->faction_bonus_id[i];
  if(id>=7&&id<=9&&!add(next.fungus[id-7],d->faction_bonus_val1[i]))return TechnologyEffectsError::overflow;
 }
 for(auto&value:next.fungus)if(value<0)value=0;
 const int32_t economy=f->SE_economy_pending;
 const int32_t bonus=(economy>2)+(economy>3)+(economy>4);
 if(!add(next.commerce,bonus))return TechnologyEffectsError::overflow;
 output=next;return TechnologyEffectsError::ok;
}
}
