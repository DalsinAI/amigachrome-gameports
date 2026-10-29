/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute late defense abilities; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_DEFENSE_ABILITIES_H
#define AC_ALPHA_DEFENSE_ABILITIES_H
#include "terrain_defense.h"
namespace alpha {
struct DefenseAbilitiesContext {
 Variant variant;int32_t defense;
 uint32_t attacker_triad,attacker_chassis_speed,defender_armor;
 bool attacker_dissociative,defender_jammer,defender_tracking;
 int32_t jammer_percent,tracking_percent;
};
struct DefenseAbilitiesStrength {int32_t defense;uint32_t applied;};
/* Read-only late normal battle stage after In Port. Explicit current effective
 * ability observations, not raw flags interpreted as live authority. Pulse is
 * SMAX only and uses armor ordinal10/12 AND land chassis speed>1. Jammer and
 * Tracking ignore Dissociative in SMAC, suppress under it in SMAX. Applied
 * bits1/2/4 report numerical branches, not four-entry UI ledger writes.
 * Checked original multiply-before-divide; error/overlap leaves output intact.
 * No prior battle calculation, legal attack, World binding, cache/UI/action.
 */
LogicError defense_abilities(const DefenseAbilitiesContext&,DefenseAbilitiesStrength&);
}
#endif
