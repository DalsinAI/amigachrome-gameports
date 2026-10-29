/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_BATTLE_ENTRY_H
#define AC_ALPHA_WORLD_BATTLE_ENTRY_H
#include "battle_entry.h"
#include "land_combat.h"
namespace alpha {
struct WorldBattleEntryRuntime {
 BattleEntryObservation observation;
 uint32_t attack_flags; int32_t mode, execution_argument;
 uint32_t multiplayer_active; int32_t attacker_report_count;
};
enum class WorldBattleEntryError { ok,bad_state,missing_runtime,unsupported,overflow };
struct WorldBattleEntryInspection {
 LandCombatStrengths before_hasty;
 uint32_t speed; BattleChargeResult charge; BattleHastyResult hasty;
};
/* Current owned World binding, ordinary nonbase/non-native land components,
 * explicit mode0/flags2/offline/execute1 observation. Nerve ability/grant paths
 * stay held. Actual actor state, design, faction, damage/morale/speed and rules
 * supply the arithmetic; caller report count is explicitly observed separately.
 * Numeric inspection only: no World/ledger mutation, label lookup, session
 * authority, best-defender selection or completed pre-round transition. The
 * returned string/report obligations MUST be resolved before caller advancement.
 */
WorldBattleEntryError inspect_world_battle_entry(const OriginalWorld&,
 const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,
 size_t defender,const WorldBattleEntryRuntime&,WorldBattleEntryInspection&);
}
#endif
