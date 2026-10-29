/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original offense_proto/armor_proto/psi_factor semantics; OpenSMACX
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, Brendan Casey2013-2021.
 * Original edition-specific instruction behavior takes precedence over mod fixes.
 */
#ifndef ALPHA_COMBAT_PROTOTYPE_H
#define ALPHA_COMBAT_PROTOTYPE_H
#include "original_world.h"
#include "saved_speed.h"
namespace alpha {
struct PrototypeCombatRatings {int32_t offense,defense;bool offense_psi,defense_psi,probe_duel;};
/* Actual opposing prototype required: no original negative-actor unchecked read
 * is reproduced. Eightfold in-battle scale; neither morale nor full modifiers,
 * legal attack eligibility, damage, kills or RNG is supplied by these leaves.
 * Signed byte ratings and byte missile/mode facts are checked conservatively.
 */
LogicError prototype_combat_ratings(Variant,const UnitDefinitions&,const BasicRules&,
 const OriginalPrototype&,size_t prototype,const OriginalPrototype&,size_t opposing,
 bool bombard,PrototypeCombatRatings&);
/* Original signed arithmetic, division toward zero. Overflowing original i32
 * intermediate products/additions are rejected before output. SMAC has no
 * Fungal Tower bonus; SMAX defense applies it after Neural Amplifier bonus. */
LogicError combat_psi_factor(Variant,int32_t strength,int32_t rule_psi,bool attacking,
 bool fungal_tower,bool neural_amplifier,bool dream_twister,int32_t&);
LogicError world_prototype_combat_ratings(const OriginalWorld&,const UnitDefinitions&,
 const BasicRules&,size_t actor,size_t opposing,bool bombard,PrototypeCombatRatings&);
LogicError world_combat_psi_factor(const OriginalWorld&,size_t actor,int32_t strength,
 bool attacking,int32_t&);
}
#endif
