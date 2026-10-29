/* SPDX-License-Identifier: GPL-3.0-or-later. See retirement_history.h. */
#include "retirement_history.h"
#include "technology_effects_ranges.h"
namespace alpha {
LogicError retirement_history_stage(const uint8_t* table,size_t size,int32_t faction,int32_t cursor,const OriginalPrototype& design,RetirementHistoryResult& out){
 if(!table || size!=retirement_history_bytes || faction<0 || faction>7 || cursor<0 || cursor>63 ||
 detail::effects_range_overlap(&out,sizeof out,table,size) || detail::effects_range_overlap(&out,sizeof out,&design,sizeof design))return LogicError::bad_state;
 RetirementHistoryResult next{};next.next_slot=(cursor+1)%64;
 for(size_t i=0;i<size;++i)next.table[i]=table[i];
 uint8_t* p=next.table+((size_t)faction*64+(size_t)cursor)*8;uint32_t abilities=design.ability_flags;
 for(unsigned i=0;i<4;++i){p[i]=(uint8_t)abilities;abilities>>=8;}
 p[4]=design.chassis_id;p[5]=design.weapon_id;p[6]=design.armor_id;p[7]=design.reactor_id;
 out=next;return LogicError::ok;
}
}
