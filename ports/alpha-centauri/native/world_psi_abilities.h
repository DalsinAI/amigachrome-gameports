/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in psi_abilities.h. */
#ifndef AC_ALPHA_WORLD_PSI_ABILITIES_H
#define AC_ALPHA_WORLD_PSI_ABILITIES_H
#include "psi_abilities.h"
namespace alpha {
/* Read-only normal flags0/non-Probe attacker inspection. Actual components,
 * edition missile/Spore psi classification and design-owner FREEABIL/technology
 * facts. Explicit early-offense and late-defense inputs retain distinct phases.
 * No complete battle/mode/session/attack authority or cache/UI/World/RNG writes.
 */
LogicError world_psi_abilities(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,int32_t offense,
 int32_t defense,PsiAbilitiesStrengths&);
}
#endif
