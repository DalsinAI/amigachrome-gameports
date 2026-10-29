/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_UNIT_READINESS_H
#define ALPHA_WORLD_UNIT_READINESS_H
#include "world_unit_stats.h"
namespace alpha {
struct UnitReadiness {bool ready,jailed,speed_evaluated;uint32_t speed;};
/* Original veh_ready + veh_jail. Immutable inspection, not End Turn admission.
 * Current human is explicit engine context; session binding derives its owner.
 * Physical base safe only when its low faction nibble is1..7 (Planet0 base is
 * NOT safe in the original); any physical airbase is safe. Boarding air carrier
 * reference is checked against actual actors before use. No modern mod fixes.
 * Speed is evaluated only after original short-circuit gates, in the inherited
 * checked0..255 domain. Negative/out-of-range actor indices are rejected.
 * Output/inputs preserve on error; alias destinations rejected before writes.
 */
LogicError world_unit_readiness(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 size_t vehicle,int32_t human,int32_t road_rate,UnitReadiness&);
}
#endif
