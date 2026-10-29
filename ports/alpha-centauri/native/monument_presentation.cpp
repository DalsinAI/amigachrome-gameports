/* SPDX-License-Identifier: GPL-3.0-or-later. See monument_presentation.h. */
#include "monument_presentation.h"
#include <stdint.h>
namespace alpha {
static uint32_t get(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool overlap(const void* a,size_t an,const void* b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return x<=y?y-x<an:x-y<bn;}
LogicError monument_presentation(const MonumentPresentationInput& in,MonumentPresentationResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || !in.local_record || in.size!=monument_record_bytes ||
 overlap(&out,sizeof out,in.local_record,in.size) || overlap(&out,sizeof out,&in,sizeof in))return LogicError::bad_state;
 MonumentPresentationResult next{0,false,false};
 if(!in.multiplayer_active && !(in.variant==Variant::alien_crossfire && (in.local_faction_rule_flags&0x80u)) &&
 (!in.new_event || (!(in.more_preferences&0x1000000u) && !(in.game_state&0x10u) && (in.game_state&9u)!=9u))){
  next.count_written=true;for(unsigned i=0;i<15;++i)if(get(in.local_record+i*56+16))++next.achieved_count;
  next.initialization_required=next.achieved_count!=0;
 }
 out=next;return LogicError::ok;
}
}
