/* SPDX-License-Identifier: GPL-3.0-or-later. See facility_notice.h. */
#include "facility_notice.h"
namespace alpha {
static bool valid_facility(Variant variant,int32_t facility){
 return (variant==Variant::base || variant==Variant::alien_crossfire) && facility>=2 && facility<=(variant==Variant::base?33:41);
}
LogicError facility_notice_entry(const FacilityNoticeEntryInput& in,FacilityNoticeRoute& out){
 if(!valid_facility(in.variant,in.facility) || in.faction<0 || in.faction>7 || in.local_faction<0 || in.local_faction>7)return LogicError::bad_state;
 FacilityNoticeRoute next;
 if(in.variant==Variant::alien_crossfire && in.facility==40)next=FacilityNoticeRoute::subspace_special;
 else if(in.faction!=in.local_faction)next=in.facility>=(in.variant==Variant::base?33:40)?FacilityNoticeRoute::foreign:FacilityNoticeRoute::none;
 else {
  const unsigned index=(unsigned)in.facility/8,mask=1u<<((unsigned)in.facility%8);
  next=!(in.announced[index]&mask) && !(in.game_state&1u) && (in.warnings&1u)?FacilityNoticeRoute::local_first:FacilityNoticeRoute::local_regular;
 }
 out=next;return LogicError::ok;
}
LogicError facility_notice_construction(const FacilityNoticeConstructionInput& in,FacilityNoticeConstructionResult& out){
 if(!valid_facility(in.variant,in.facility) || (in.variant==Variant::alien_crossfire && in.facility==40))return LogicError::bad_state;
 FacilityNoticeConstructionResult next{};
 for(unsigned i=0;i<12;++i)next.announced[i]=in.announced[i];
 next.blurb_requested=(in.warnings&1u)!=0;
 if(next.blurb_requested)next.announced[(unsigned)in.facility/8]|=(uint8_t)(1u<<((unsigned)in.facility%8));
 out=next;return LogicError::ok;
}
LogicError facility_notice_voice(const FacilityNoticeVoiceInput& in,bool& out){
 if(in.faction<0 || in.faction>7)return LogicError::bad_state;
 out=(in.preferences&0x4000u) && (in.human_mask&(1u<<(unsigned)in.faction));return LogicError::ok;
}
bool facility_notice_fade(uint32_t current_preferences){return (current_preferences&0x24000u)==0x24000u;}
LogicError facility_notice(const FacilityNoticeInput& in,FacilityNoticeResult& out){
 FacilityNoticeEntryInput entry{in.variant,in.facility,in.faction,in.local_faction,in.game_state,in.warnings_entry,{}};
 for(unsigned i=0;i<12;++i)entry.announced[i]=in.announced[i];
 FacilityNoticeResult next{};
 if(facility_notice_entry(entry,next.route)!=LogicError::ok)return LogicError::bad_state;
 for(unsigned i=0;i<12;++i)next.announced[i]=in.announced[i];
 if(next.route==FacilityNoticeRoute::local_first){
  FacilityNoticeConstructionInput construction{in.variant,in.facility,in.warnings_after_construction,{}};
  for(unsigned i=0;i<12;++i)construction.announced[i]=in.announced[i];
  FacilityNoticeConstructionResult built{};
  if(facility_notice_construction(construction,built)!=LogicError::ok)return LogicError::bad_state;
  for(unsigned i=0;i<12;++i)next.announced[i]=built.announced[i];
  next.blurb_requested=built.blurb_requested;
  if(facility_notice_voice({in.faction,in.preferences_before_voice,in.human_mask},next.voice_load_requested)!=LogicError::ok)return LogicError::bad_state;
  next.voice_fade_requested=facility_notice_fade(in.preferences_after_popup);
 }
 out=next;return LogicError::ok;
}
}
