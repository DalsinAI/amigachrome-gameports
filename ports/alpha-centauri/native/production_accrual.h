/* SPDX-License-Identifier: GPL-3.0-or-later
 * Pre-production arithmetic independently adapted from Thinker base.cpp,
 * MIT notice in THINKER-LICENSE.txt, pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Original instruction facts retain edition sentinels and pre-abort ordering.
 */
#ifndef ALPHA_PRODUCTION_ACCRUAL_H
#define ALPHA_PRODUCTION_ACCRUAL_H
#include "game_logic.h"
namespace alpha {
struct ProductionAccrualInput {
 Variant variant;uint32_t state_flags;
 int32_t mineral_intake_2,mineral_surplus,nutrients_accumulated;
 int32_t minerals_accumulated,item,best_mineral_output;
};
struct ProductionAccrualResult {
 uint32_t state_flags;
 int32_t best_mineral_output,minerals_accumulated;
 bool negative_food_abort,accrual_branch;
};
/* Pure pre-production dependency, NOT a turn/phase authority or World mutation.
 * Clears production-done0x800000 and updates historical mineral peak BEFORE the
 * negative-food abort. Nonnegative surplus accrues only without riot bit2 and
 * outside the edition's stockpile-energy item (-38 SMAC/-69 SMAX).
 * Each original signed intermediate, including doubled surplus, must fit int32.
 * Failure preserves output rather than reproducing an unsafe partial update.
 * No eligibility, spending, production reset, completion, queue, new unit,
 * project, research, or once-per-turn claim. Caller must provide actual current
 * phase facts and guarantee single execution before any future commit.
 */
LogicError production_accrual(const ProductionAccrualInput&,ProductionAccrualResult&);
}
#endif
