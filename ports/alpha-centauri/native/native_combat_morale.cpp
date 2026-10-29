/* SPDX-License-Identifier: GPL-3.0-or-later. See native_combat_morale.h. */
#include "native_combat_morale.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError native_combat_morale(Variant variant,const NativeCombatMoraleContext& c,uint32_t& out){
 if((variant!=Variant::base&&variant!=Variant::alien_crossfire)||c.unit_id<0||c.unit_id>=512||c.opponent_faction< -1||c.opponent_faction>7||c.flags>65535||c.fungus_neighbors>8||detail::effects_range_overlap(&c,sizeof c,&out,sizeof out))return LogicError::bad_state;
 int32_t m=c.turn<45?0:c.turn<90?1:c.turn<170?2:c.turn<250?3:c.turn<330?4:6;
 if(c.opponent_faction>0){m+=(c.major_atrocities!=0);if(variant==Variant::alien_crossfire)m+=(c.tectonic_detonations!=0);}
 if(c.state&0x100u)++m;
 if(c.unit_id==10)++m;
 m+=(c.flags>>3)&3u;
 if(variant==Variant::alien_crossfire&&c.unit_id==19)m=(int32_t)c.fungus_neighbors-2;
 out=(uint32_t)(m<0?0:m>6?6:m);return LogicError::ok;
}
}
