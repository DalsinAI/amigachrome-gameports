/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in original_human_cost.h. */
#include "original_human_cost.h"
namespace alpha {
LogicError original_human_costs(const OriginalHumanCostInput& in,OriginalHumanCosts& output){
 if(in.nutrient_multiplier<1 || in.nutrient_multiplier>100 ||
    in.mineral_multiplier<1 || in.mineral_multiplier>100 || in.map_size<0 || in.map_size>6)
  return LogicError::bad_state;
 int32_t n=in.nutrient_multiplier,m=in.mineral_multiplier;
 if(in.map_size==0){n=n*8/10;m=m*8/10;}
 else if(in.map_size==1){n=n*9/10;m=m*9/10;}
 const int64_t grown=(int64_t)in.pending_growth+2*in.children_creche+2*in.golden_age;
 if(grown>2147483647)return LogicError::overflow;
 const int32_t growth=grown< -2?-2:grown>5?5:(int32_t)grown;
 n=(n*(10-growth)+9)/10;
 const int32_t industry=in.pending_industry;
 if(industry>=-7 && industry<=-3)m=(m*13+9)/10;
 else if(industry==-2)m=(m*6+4)/5;
 else if(industry==-1)m=(m*11+9)/10;
 else if(industry==0){}
 else if(industry==1)m=(m*9+9)/10;
 else if(industry==2)m=(m*4+4)/5;
 else if(industry==3)m=(m*7+9)/10;
 else if(industry==4)m=(m*3+4)/5;
 else m=(m+1)/2;
 output={n,m};return LogicError::ok;
}
}
