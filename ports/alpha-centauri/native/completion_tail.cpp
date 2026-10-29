/* SPDX-License-Identifier: GPL-3.0-or-later. See completion_tail.h. */
#include "completion_tail.h"
namespace alpha {
LogicError completion_tail(const CompletionTailInput& in,CompletionTailResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.facility<2 ||
 in.facility>=(in.variant==Variant::base?34:65) || in.queue_size<0 || in.queue_size>9)return LogicError::bad_state;
 CompletionTailResult next{in.clean_minerals,in.current_item,in.player_flags,false,false};
 if(in.facility==13 || in.facility==14 || in.facility==31 || in.facility==32){
  if(in.clean_minerals==2147483647)return LogicError::overflow;
  if(in.clean_minerals)++next.clean_minerals;
  next.player_flags&=~0x4000u;
 }
 const bool governor=(in.governor_flags&0x80000001u)==0x80000001u;
 next.base_reset_required=!in.human || (governor && !in.queue_admitted);
 if(in.human && !governor && in.current_item==-in.facility && !in.queue_size){
  next.current_item=in.variant==Variant::base?-38:-69;next.stockpile_fallback=true;
 }
 out=next;return LogicError::ok;
}
}
