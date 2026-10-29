/* SPDX-License-Identifier: GPL-3.0-or-later; source attribution cost_factor.h. */
#include "world_cost_factor.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 if(!a||!b||!an||!bn)return false;
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return x<=y?y-x<an:x-y<bn;
}
bool alias(const OriginalWorld&w,const BasicRules&r,const void*out,size_t n){
 if(overlap(out,n,&w,sizeof w)||overlap(out,n,&r,sizeof r))return true;
 if(!w.loaded())return false;
 const auto&g=w.game();
 return overlap(out,n,w.original_bytes(),w.original_size())||overlap(out,n,w.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile))||overlap(out,n,w.base(0),(size_t)g.base_count*sizeof(OriginalBase))||overlap(out,n,w.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle))||overlap(out,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(out,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(out,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));
}
}
CostFactorError world_cost_factor(const OriginalWorld&w,const BasicRules&r,
 int32_t faction,uint32_t resource,int32_t base,int32_t&out){
 if(alias(w,r,&out,sizeof out))return CostFactorError::bad_buffer;
 if(!w.loaded()||faction<1||faction>7||base< -1)return CostFactorError::bad_state;
 const auto*f=w.faction((size_t)faction);if(!f)return CostFactorError::bad_state;
 const auto*b=base>=0?w.base((size_t)base):nullptr;
 if(base>=0&&(!b||b->faction_id!=faction))return CostFactorError::bad_state;
 if(!(w.game().preferences.FactionStatus[0]&(1u<<(unsigned)faction)))return CostFactorError::unsupported;
 const int32_t multiplier=resource?r.mineral_cost_multi:r.nutrient_cost_multi;
 if(multiplier<1||multiplier>100)return CostFactorError::bad_state;
 const CostFactorFacts facts{resource,(uint32_t)multiplier,(uint32_t)w.game().map.info.world_settings[0],(uint32_t)w.game().preferences.DiffLevel,
  f->SE_industry_pending,f->SE_growth_pending,true,false,false,false,base>=0,
  b&&(b->facilities_built[0]&4u),b&&((uint32_t)b->state_flags&4u)};
 return cost_factor(facts,out);
}
CostFactorError world_base_cost_factor(const OriginalWorld&w,const BasicRules&r,
 size_t base,uint32_t resource,int32_t&out){
 if(alias(w,r,&out,sizeof out))return CostFactorError::bad_buffer;
 if(!w.loaded()||base>2147483647u)return CostFactorError::bad_state;
 const auto*b=w.base(base);if(!b)return CostFactorError::bad_state;
 return world_cost_factor(w,r,b->faction_id,resource,(int32_t)base,out);
}
}
