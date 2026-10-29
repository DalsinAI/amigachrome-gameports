/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original battle_compute psi ability arithmetic; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 */
#ifndef AC_ALPHA_PSI_ABILITIES_H
#define AC_ALPHA_PSI_ABILITIES_H
#include "terrain_defense.h"
namespace alpha {
struct PsiAbilitiesContext {
 Variant variant;int32_t offense,defense;uint32_t psi_mask;
 uint32_t attacker_weapon,defender_armor,attacker_plan;
 bool attacker_dissociative,defender_dissociative,attacker_empath,defender_trance;
 int32_t empath_percent,trance_percent;
};
struct PsiAbilitiesStrengths {int32_t offense,defense;uint32_t applied;};
/* Both actors present, normal flags0, attacking plan not11 (Probe). Offense
 * input is BEFORE early Resonance/Empath; defense input is AFTER terrain/base/
 * sensor and BEFORE Trance/Resonance. These independent phase inputs are
 * explicit, not a whole battle composition. Psi mask1=attacker psi weapon,
 * mask2=defender psi armor. Applied bits1/2/4/8 = ResAtk/Empath/Trance/ResDef.
 * No mode/attack/session authority, UI ledger/cache writes or World mutation.
 * Checked signed multiply-before-divide; errors/aliases preserve output.
 */
LogicError psi_abilities(const PsiAbilitiesContext&,PsiAbilitiesStrengths&);
}
#endif
