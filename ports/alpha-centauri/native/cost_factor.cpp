/* SPDX-License-Identifier: GPL-3.0-or-later; attribution in cost_factor.h. */
#include "cost_factor.h"
#include <stddef.h>
namespace alpha {
CostFactorError cost_factor(const CostFactorFacts&f,int32_t&out){
 const uintptr_t a=(uintptr_t)&f,b=(uintptr_t)&out;
 if((a<=b&&b-a<sizeof f)||(b<a&&a-b<sizeof out))return CostFactorError::bad_buffer;
 if(f.multiplier<1||f.multiplier>100||(!f.human&&f.difficulty>5))return CostFactorError::bad_state;
 static const uint32_t ai[]={13,12,11,10,8,7};
 uint32_t factor=f.human?10:ai[f.difficulty]-(unsigned)f.leader_threat-(unsigned)(!f.multiplayer_net&&f.leader_human);
 if(f.multiplier!=10)factor=factor*f.multiplier/10;
 if(f.map_size==1)factor=factor*9/10;
 else if(f.map_size==0)factor=factor*8/10;
 if(f.resource==1){
  switch(f.industry_pending){
   case -7:case -6:case -5:case -4:case -3:factor=(factor*13+9)/10;break;
   case -2:factor=(factor*6+4)/5;break;
   case -1:factor=(factor*11+9)/10;break;
   case 0:break;
   case 1:factor=(factor*9+9)/10;break;
   case 2:factor=(factor*4+4)/5;break;
   case 3:factor=(factor*7+9)/10;break;
   case 4:factor=(factor*3+4)/5;break;
   default:factor=(factor+1)/2;break;
  }
 }else if(!f.resource){
  const int64_t adjusted=(int64_t)f.growth_pending+(f.base_present?2*(int)f.children_creche+2*(int)f.golden_age:0);
  if(adjusted<(-2147483647LL-1)||adjusted>2147483647LL)return CostFactorError::overflow;
  const int32_t growth=adjusted< -2?-2:adjusted>5?5:(int32_t)adjusted;
  factor=(factor*(uint32_t)(10-growth)+9)/10;
 }
 out=(int32_t)factor;return CostFactorError::ok;
}
}
