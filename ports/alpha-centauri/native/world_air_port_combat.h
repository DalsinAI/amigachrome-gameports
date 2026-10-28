/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in air_port_combat.h. */
#ifndef AC_ALPHA_WORLD_AIR_PORT_COMBAT_H
#define AC_ALPHA_WORLD_AIR_PORT_COMBAT_H
#include "air_port_combat.h"
namespace alpha {
/* Read-only normal conventional combat inspection. Actual current actors,
 * edition, design-owner FREEABIL prerequisites, chassis missile and base facts.
 * Psi components and stale base bits reject. Incoming strengths stay explicit;
 * no legal attack, live authority, cache/UI/action/World/RNG writes.
 */
LogicError world_air_port_combat(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,int32_t offense,
 int32_t defense,AirPortStrengths&);
}
#endif
