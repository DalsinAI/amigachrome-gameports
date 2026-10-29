/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_BATTLE_REPORTING_H
#define AC_ALPHA_WORLD_BATTLE_REPORTING_H
#include "world_battle_entry.h"
#include "attack_modifier_ledger.h"
namespace alpha {
struct WorldBattleReportingRuntime {
 BattleEntryObservation observation;uint32_t attack_flags;
 int32_t mode,execution_argument;uint32_t multiplayer_active;
};
struct WorldBattleReportingInspection {
 WorldBattleEntryInspection entry;
 AttackModifierResult modifiers;
 AttackModifierLedger final_ledger;uint32_t lookup_count;
};
/* Derives source-order modifier facts and count from CURRENT World stages,
 * then supplies that derived count to the frozen entry inspection. No caller
 * report count or zero fallback exists. Prior unused symbolic slots survive;
 * optional hasty label339 follows battle_compute entries. Every symbolic label
 * lookup/copy remains pending. Same conservative ordinary scope as World entry:
 * no World/ledger/text/UI mutation, legal attack or caller advancement. */
WorldBattleEntryError inspect_world_battle_reporting(const OriginalWorld&,
 const UnitDefinitions&,const Rules&,const BasicRules&,size_t attacker,size_t defender,
 const WorldBattleReportingRuntime&,const AttackModifierLedger&prior,
 WorldBattleReportingInspection&);
}
#endif
