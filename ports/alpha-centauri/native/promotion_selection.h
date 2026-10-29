/* SPDX-License-Identifier: GPL-3.0-or-later. Original battle_fight_2 bounded
 * selection prefix51e193/50a1b3. No callback or random result is substituted. */
#ifndef AC_ALPHA_PROMOTION_SELECTION_H
#define AC_ALPHA_PROMOTION_SELECTION_H
#include "combat_round.h"
namespace alpha {
enum class SelectionObservation {missing,supplied};
enum class SelectionOutcome {attacker_lost,attacker_won};
enum class PromotionIntent {none,promote,random_required};
struct PromotionSelectionContext {
 SelectionObservation observation;SelectionOutcome outcome;
 CombatHealthContext attacker,defender;
 int32_t attacker_strength,defender_strength;
 /* The ORIGINAL pre-round remaining health locals, not the post-round values.
  * A live owning caller must retain them. Zero divisors are not fabricated. */
 uint32_t initial_attacker_health,initial_defender_health,selected_effective_morale;
 bool target_base_present;
};
struct PromotionSelectionResult {PromotionIntent intent;uint32_t recipient;int32_t attacker_strength,defender_strength;};
/* Inspection of already supplied original caller locals. Raw reactor multiplies
 * wrap32 before signed division. The loser plan12 suppresses promotion. A
 * pending actual source random call is an explicit boundary, not zero roll.
 * recipient0/1 identifies attacker/defender only when promote was reached.
 * No World mutation, helper promotion, random advancement or legal attack. */
LogicError inspect_promotion_selection(const PromotionSelectionContext&,PromotionSelectionResult&);
}
#endif
