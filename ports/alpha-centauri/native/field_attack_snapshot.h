/* SPDX-License-Identifier: GPL-3.0-or-later
 * Pinned Thinker default battle_fight/battle_kill/veh_kill/promote policy,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af. FIELD-ATTACK.md.
 */
#ifndef ALPHA_FIELD_ATTACK_SNAPSHOT_H
#define ALPHA_FIELD_ATTACK_SNAPSHOT_H
#include "world_combat_crt.h"
namespace alpha {
enum class FieldAttackError {ok,bad_state,bad_buffer,unsupported,overflow,out_of_memory,bad_snapshot,bad_reference};
struct FieldAttackStage {
 CombatRandomState combat_random;
 int32_t combat_wins[8],combat_losses[8];
};
struct FieldAttackReceipt {
 size_t bytes;
 uint32_t movement_random_seed;
 FieldAttackStage next;
 FieldCombatFacts facts;
 CombatResult combat;
 int32_t attacker_before,defender_before,removed_before,winner_after;
 int32_t winner_faction,loser_faction,selected_vehicle_after;
 uint32_t promotion_draws;
 bool promoted,recompute_required,selection_remap_required,presentation_required;
};
/* Current-world stackless field BATTLE phase transaction. Successful output is
 * an entire reparsed container with exactly one52-byte casualty record removed,
 * repaired references/counters/map occupancy, real damage/movement/promotion,
 * and next explicit CRT/runtimecombat ledger in the receipt. Source world,
 * original input bytes and stage are never mutated. Output/receipt commit only
 * after all validation; unsupported/overflow/alias/short buffer leaves both
 * unchanged. Preserve qualified current live movement/orders/facility writes,
 * and reject other unencoded live changes rather than reverting to raw save.
 * No UI/LocalPlayer or persistent command authority is installed. Owning engine
 * must atomically install AFTERbytes + next ledger under its revision authority.
 * Attacker does not advance into the defender tile in source battle_fight;
 * movement command continuation remains a separate phase, not fabricated here.
 * First-kill monument creation, objectives/assistant events, third-party terrain
 * diplomacy and other unqualified branches reject (see FIELD-ATTACK.md).
 */
size_t field_attack_snapshot_size(const OriginalWorld&);
FieldAttackError encode_field_attack_snapshot(const OriginalWorld&,const UnitDefinitions&,
 const Rules&,const BasicRules&,size_t attacker,size_t defender,const FieldAttackStage&,
 void*output,size_t capacity,FieldAttackReceipt&);
const char* field_attack_error_name(FieldAttackError);
}
#endif
