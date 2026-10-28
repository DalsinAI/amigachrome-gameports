/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2;
 * OpenSMACX pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Casey2013-2021. */
#ifndef AC_ALPHA_RETREAT_DESTINATION_H
#define AC_ALPHA_RETREAT_DESTINATION_H
#include "world_zoc.h"
#include "supply_pods.h"
#include "rules_text.h"
namespace alpha {
struct RetreatDestination {int32_t direction,x,y;uint32_t eligible_mask;};
/* Read-only ordinary nonmissile LAND defender selection. Explicit network and
 * coefficients are facts, not authority. First source-direction tie wins.
 * Map-flat metadata word must be canonical 0/1 (shared importer limitation).
 * No threshold/countdown, battle/retreat action, relocation or completion. */
LogicError world_land_retreat_destination(const OriginalWorld&,const UnitDefinitions&,size_t defender,uint32_t incoming_direction,bool multiplayer_active,const PodCoefficients&,RetreatDestination&);
}
#endif
