/* SPDX-License-Identifier: GPL-3.0-or-later. See unit_availability.h. */
#include "unit_availability.h"
namespace alpha {
LogicError unit_availability(const UnitAvailabilityInput& in,bool& out){
 if((in.variant!=Variant::base && in.variant!=Variant::alien_crossfire) || in.unit<0 || in.unit>=512 || in.faction<0 || in.faction>=8 || in.triad>2)return LogicError::bad_state;
 bool available=(in.flags&1u)!=0 && !(in.obsolete&(1u<<in.faction));
 if(in.unit<64){
  const bool tech=in.faction>0 && (in.technology==-1 || (in.technology>=0 && in.technology<88 && in.prerequisite1> -2 && (in.prerequisite2> -2 || in.prerequisite1==-1) && (in.technology_owners&(1u<<in.faction))));
  available=available && tech;
 }
 if(in.plan==8 && (in.game_rules&0x400000u))available=false;
 if(in.base_present && in.triad==1 && !in.coastal)available=false;
 if(in.variant==Variant::alien_crossfire && !in.expansion_enabled){
  if(in.armor>9 || in.weapon==12 || in.weapon==13 || in.weapon==14 || in.weapon==24 || in.weapon==25 || (in.abilities&0x1f000000u) || (in.unit>=14 && in.unit<=20))available=false;
 }
 if(in.unit>=64 && in.unit/64!=in.faction)available=false;
 out=available;return LogicError::ok;
}
}
