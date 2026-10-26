/* SPDX-License-Identifier: GPL-3.0-or-later. See sensor_combat.h. */
#include "sensor_combat.h"
namespace alpha {
LogicError sensor_combat_adjustment(Variant v,int32_t defense,uint32_t coverage,int32_t percent,SensorCombatAdjustment&out){
 if((v!=Variant::base&&v!=Variant::alien_crossfire)||(coverage!=0&&coverage!=1&&coverage!=3)||(v==Variant::base&&coverage>1))return LogicError::bad_state;
 int32_t result=defense;
 for(uint32_t bit=1;bit<=2;bit<<=1)if(coverage&bit){const int64_t factor=(int64_t)percent+100;if(factor<(-2147483647LL-1)||factor>2147483647LL)return LogicError::overflow;const int64_t n=factor*result;if(n<(-2147483647LL-1)||n>2147483647LL)return LogicError::overflow;result=(int32_t)(n/100);}
 out=SensorCombatAdjustment{result,coverage};return LogicError::ok;
}
}
