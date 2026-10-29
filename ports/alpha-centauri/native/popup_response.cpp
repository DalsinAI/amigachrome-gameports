/* SPDX-License-Identifier: GPL-3.0-or-later. See popup_response.h. */
#include "popup_response.h"
namespace alpha {
LogicError popup_response(const PopupResponseInput& in,PopupResponseResult& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.runtime!=PopupResponseRuntime::owned_offline)return LogicError::bad_state;
 PopupResponseResult next{in.base_upkeep_flag,in.warnings,in.warnings_mirror,in.governor_flags,false,false};
 if(in.disable_warning_checked){next.warnings&=~in.warning_mask;next.warnings_mirror=next.warnings;next.preferences_save_required=true;}
 else if(in.confirmed_result==0)next.base_upkeep_flag=1;
 else if(in.confirmed_result==2)next.governor_flags|=0x80000040u;
 else if(in.confirmed_result==3)next.staple_required=true;
 out=next;return LogicError::ok;
}
}
