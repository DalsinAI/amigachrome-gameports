/* SPDX-License-Identifier: GPL-3.0-or-later. See facility_notice.h. */
#include "facility_notice.h"
namespace alpha {
LogicError facility_notice(const FacilityNoticeInput& in,FacilityNoticeResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.facility<2 ||
 in.facility>(in.variant==Variant::base?33:41) || in.faction<0 || in.faction>7 || in.local_faction<0 || in.local_faction>7)return LogicError::bad_state;
 FacilityNoticeResult next{};for(unsigned i=0;i<12;++i)next.announced[i]=in.announced[i];
 const bool local=in.faction==in.local_faction;
 if(in.variant==Variant::alien_crossfire && in.facility==40)next.route=FacilityNoticeRoute::subspace_special;
 else if(!local)next.route=in.facility>=(in.variant==Variant::base?33:40)?FacilityNoticeRoute::foreign:FacilityNoticeRoute::none;
 else {
  const unsigned index=(unsigned)in.facility/8,mask=1u<<((unsigned)in.facility%8);
  const bool first=!(in.announced[index]&mask) && !(in.game_state&1u) && (in.warnings_entry&1u);
  next.route=first?FacilityNoticeRoute::local_first:FacilityNoticeRoute::local_regular;
  if(first){
   next.blurb_requested=(in.warnings_after_construction&1u)!=0;
   if(next.blurb_requested)next.announced[index]|=(uint8_t)mask;
   next.voice_load_requested=(in.preferences_before_voice&0x4000u) && (in.human_mask&(1u<<(unsigned)in.faction));
   next.voice_fade_requested=(in.preferences_after_popup&0x24000u)==0x24000u;
  }
 }
 out=next;return LogicError::ok;
}
}
