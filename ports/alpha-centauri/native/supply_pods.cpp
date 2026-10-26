/* SPDX-License-Identifier: GPL-3.0-or-later. See supply_pods.h. */
#include "supply_pods.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError tile_supply_pod(const OriginalMapInfo&m,const OriginalMapTile&t,int32_t x,int32_t y,uint32_t rules,const PodCoefficients&c,uint32_t&out){
 using detail::effects_range_overlap;
 if(effects_range_overlap(&m,sizeof m,&out,sizeof out)||effects_range_overlap(&t,sizeof t,&out,sizeof out)||effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 if(m.width<2||m.width>4096||(m.width&1)||!m.height||m.height>4096||x<0||y<0||(uint32_t)x>=m.width||(uint32_t)y>=m.height||((x+y)&1))return LogicError::bad_state;
 uint32_t result=0;
 if(!(t.items&0x102000u)){
  if(rules&0x2000u)result=(t.items&0xc000000u)?2u:0u;
  else if(t.items&0x10000000u)result=1;
  else if(m.random_seed){
   const int32_t avg=(x+y)/2,diff=x-avg;
   const uint32_t code=((uint32_t)avg&3u)+4u*((uint32_t)diff&3u);
   const int32_t floor_diff=diff<0?-((-diff+3)/4):diff/4;
   const uint32_t land=(m.random_seed-5u*(uint32_t)((x+y)/8)-3u*(uint32_t)floor_diff)&15u;
   if((t.climate&0xe0u)>=0x60u&&code==land)result=2;
   else{const uint32_t generated=(m.random_seed+8u+(uint32_t)(diff/4)*(uint32_t)c.difference+(uint32_t)(avg/4)*(uint32_t)c.average)&31u;result=code==generated?1u:0u;}
  }
 }
 out=result;return LogicError::ok;
}
}
