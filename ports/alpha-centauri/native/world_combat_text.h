/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_COMBAT_TEXT_H
#define AC_ALPHA_WORLD_COMBAT_TEXT_H
#include "combat_labels.h"
namespace alpha {
enum class WorldCombatTextError {ok,bad_state,missing_runtime,unsupported,overflow,missing_label,text_too_long};
struct WorldCombatTextInspection {WorldBattleReportingInspection numeric;CombatTextReporting text;};
/* One checked current-World numeric -> original-label lookup/copy boundary.
 * Derives report count, resolves ALL entered lookups before output, and retains
 * original prior text buffer tails. Labels/profile/runtime are explicit inputs.
 * No World/RNG/actor mutation, GUI acknowledgement, legal attack, caller/round
 * advancement or save/turn authority. Same conservative World reporting scope.
 */
WorldCombatTextError inspect_world_combat_text(const OriginalWorld&,
 const UnitDefinitions&,const Rules&,const BasicRules&,size_t,size_t,
 const WorldBattleReportingRuntime&,const AttackModifierLedger&,
 const OriginalLabelsView&,const CombatTextLedger&,WorldCombatTextInspection&);
}
#endif
