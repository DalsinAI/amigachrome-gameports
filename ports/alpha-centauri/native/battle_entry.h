/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_BATTLE_ENTRY_H
#define AC_ALPHA_BATTLE_ENTRY_H
#include "rules_text.h"
namespace alpha {
enum class BattleEntryObservation { missing, supplied };
enum class BattleEntryError { ok, bad_state, missing_runtime, unsupported };
struct BattleChargeFacts {
 BattleEntryObservation observation; Variant edition;
 int32_t mode, road; uint32_t speed, moves_spent;
};
struct BattleChargeResult { int32_t charge; uint32_t speed_used; };
struct BattleHastyFacts {
 BattleEntryObservation observation; Variant edition;
 uint32_t attack_flags, attacker_state, defender_state, base_present;
 uint32_t attacker_faction, attacker_design, moves_spent, speed;
 int32_t mode, road, charge, offense, report_count;
};
struct BattleHastyResult {
 uint32_t attacker_state, defender_state, moves_spent, scaled;
 int32_t offense, penalty, report_count;
 uint32_t string_lookup_required, report_append_required;
};
/* Two independently observed original caller fragments. Speed and frame facts
 * must describe their respective observation points: battle_compute and other
 * caller work separate these fragments. No authority or continuity is inferred.
 * Hasty scope requires attack_flags==2 and positive road. String lookup/report
 * obligations are returned; no label, GUI callback or acknowledgement is faked.
 * No World mutation, complete attack, combat-round integration or turn commit.
 */
BattleEntryError inspect_battle_charge(const BattleChargeFacts&, BattleChargeResult&);
BattleEntryError inspect_battle_hasty(const BattleHastyFacts&, BattleHastyResult&);
}
#endif
