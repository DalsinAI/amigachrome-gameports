/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute artillery stage; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_ARTILLERY_COMBAT_H
#define AC_ALPHA_ARTILLERY_COMBAT_H
#include "terrain_defense.h"
namespace alpha {
struct ArtilleryContext {
 int32_t offense,defense;uint32_t attacker_triad,defender_triad;
 uint32_t attacker_altitude_byte,defender_altitude_byte,battle_flags;
 int32_t land_gun_percent,altitude_percent;
};
struct ArtilleryStrengths {int32_t offense,defense;uint32_t applied;};
/* Both actors present, explicit artillery inspection with battle_flags0..31 with bit0x2 set.
 * Flags0x18 skip this stage. Land/sea pairs take the land gun branch (even when
 * its percentage is zero); other pairs give the higher side an altitude bonus.
 * No normal/psi mode inference, legal artillery/range/session authority.
 * Applied bits1/2/4/8 = attacking gun/defending gun/attacking alt/defending alt;
 * no UI ledger writes. Checked original int32 arithmetic, errors preserve out.
 */
LogicError artillery_combat(const ArtilleryContext&,ArtilleryStrengths&);
}
#endif
