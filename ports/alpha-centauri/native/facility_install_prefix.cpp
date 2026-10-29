/* SPDX-License-Identifier: GPL-3.0-or-later. See facility_install_prefix.h. */
#include "facility_install_prefix.h"
#include <string.h>
namespace alpha {
LogicError facility_install_prefix(const FacilityInstallPrefixInput& in,FacilityInstallPrefixResult& output){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) ||
 in.facility_id<2 || in.facility_id>=(in.variant==Variant::base?34:65) ||
 in.mineral_requirement<0 || in.minerals_accumulated<in.mineral_requirement ||
 in.retool_exemption<0 || in.retool_exemption>1000)return LogicError::bad_state;
 const unsigned byte=(unsigned)in.facility_id/8,bit=1u<<((unsigned)in.facility_id%8);
 if(in.facilities[byte]&bit)return LogicError::bad_state;
 int32_t remaining=in.minerals_accumulated-in.mineral_requirement;
 const int32_t cap=in.mineral_surplus<in.retool_exemption?in.mineral_surplus:in.retool_exemption;
 if(in.human && remaining>cap)remaining=cap<0?0:cap;
 FacilityInstallPrefixResult next{};next.state_flags=(in.state_flags|0x800000u)&0x7fffffffu;
 next.minerals_accumulated=remaining;next.minerals_accumulated_2=remaining;
 memcpy(next.facilities,in.facilities,sizeof next.facilities);next.facilities[byte]|=(uint8_t)bit;
 next.network_node_tail_required=in.facility_id==8;output=next;return LogicError::ok;
}
}
