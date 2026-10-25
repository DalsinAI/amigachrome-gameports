/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_production_retool.h"
#include <string.h>
namespace alpha {
LogicError world_production_retool(const OriginalWorld& w,const BasicRules& rules,Variant profile,
 size_t base_id,int32_t proposed,ProductionRetoolResult& output){
 if(!w.loaded() || (profile!=Variant::base && profile!=Variant::alien_crossfire) ||
    ((w.game().data_version<11)!=(profile==Variant::base)))return LogicError::bad_state;
 const auto* b=w.base(base_id);if(!b || b->faction_id<0 || b->faction_id>7)return LogicError::bad_state;
 ProductionRetoolInput in{profile,rules.retool_strictness,rules.retool_pct_pen_prod_chg,rules.retool_exemption,
  (w.game().preferences.FactionStatus[0]&(1u<<(unsigned)b->faction_id))!=0,
  b->production_id_last,proposed,b->minerals_accumulated_2,{}};
 memcpy(in.facilities,b->facilities_built,sizeof in.facilities);return production_retool(in,output);
}
}
