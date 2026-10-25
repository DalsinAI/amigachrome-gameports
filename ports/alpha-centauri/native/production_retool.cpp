/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in production_retool.h. */
#include "production_retool.h"
namespace alpha {
namespace {
bool built(const ProductionRetoolInput& in,int id){return (in.facilities[(unsigned)id/8]&(1u<<((unsigned)id%8)))!=0;}
int32_t classify(const ProductionRetoolInput& in,int32_t item,int repeat,int project){
 int strict=in.strictness;if(built(in,10) && strict>0)strict=1;
 if(item<0 && in.queued_item<0 && -in.queued_item<repeat && built(in,-in.queued_item))return -1;
 if(strict==0)return 0;
 if(strict==1){if(item>=0)return 0;if(item<=-project)return 1;return (item>-repeat)+2;}
 if(strict==2 && item<=-project)return -project;
 return item;
}
}
LogicError production_retool(const ProductionRetoolInput& in,ProductionRetoolResult& output){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.strictness<0 || in.strictness>3 ||
    in.penalty_percent<0 || in.penalty_percent>100 || in.exempt_minerals<0 || in.exempt_minerals>1000 ||
    in.queued_item==(-2147483647-1))return LogicError::bad_state;
 const int repeat=in.variant==Variant::base?34:65,project=in.variant==Variant::base?39:70;
 ProductionRetoolResult next{classify(in,in.previous_item,repeat,project),classify(in,in.queued_item,repeat,project),0};
 if(in.penalty_percent && in.human && next.previous_class!=next.queued_class && in.minerals_accumulated_2>in.exempt_minerals){
  const int32_t difference=in.minerals_accumulated_2-in.exempt_minerals;
  const int64_t product=(int64_t)difference*(100-in.penalty_percent);
  if(product>2147483647)return LogicError::overflow;
  next.mineral_loss=difference-(int32_t)product/100;
 }
 output=next;return LogicError::ok;
}
}
