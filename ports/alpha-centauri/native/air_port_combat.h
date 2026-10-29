/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute air/noncombat/InPort stage; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_AIR_PORT_COMBAT_H
#define AC_ALPHA_AIR_PORT_COMBAT_H
#include "terrain_defense.h"
namespace alpha {
struct AirPortContext {
 int32_t offense,defense;uint32_t attacker_triad,defender_triad;
 bool attacker_superiority,defender_superiority,attacker_missile,defender_missile;
 int32_t defender_weapon,defender_armor,attacker_faction;bool defender_base;
 int32_t ground_strike_penalty,air_superiority_percent,noncombat_penalty,port_percent;
};
struct AirPortStrengths {int32_t offense,defense;uint32_t applied;};
/* Conventional normal-combat stage after sensor/Trance/Resonance and before
 * Pulse/Jammer/Tracking. Explicit current facts and incoming strengths only.
 * Applied bits1/2/4/8/16 = ground strike/air attack/air defense/noncombat/port.
 * This does not write a four-entry UI ledger or derive legal battle authority.
 * Checked original signed multiply-before-divide; errors/alias leave output.
 */
LogicError air_port_combat(const AirPortContext&,AirPortStrengths&);
}
#endif
