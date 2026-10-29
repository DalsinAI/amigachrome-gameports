/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original completion fragments independently adapted from Thinker base.cpp
 * (MIT, THINKER-LICENSE.txt), pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef ALPHA_COMPLETION_TAIL_H
#define ALPHA_COMPLETION_TAIL_H
#include "game_logic.h"
namespace alpha {
struct CompletionTailInput {
 Variant variant;int32_t facility,clean_minerals,current_item,queue_size;
 uint32_t player_flags,governor_flags;bool human,queue_admitted;
};
struct CompletionTailResult {int32_t clean_minerals,current_item;uint32_t player_flags;bool base_reset_required,stockpile_fallback;};
/* Pure ordinary-facility tail dependencies only, not complete production.
 * Original eco facilities13/14/31/32 increment clean_minerals ONLY when nonzero
 * and clear player flag0x4000; checked overflow rejects transactionally. Later
 * mod's forced increment from zero is deliberately excluded.
 * Reports original base_reset obligation (AI or managing governor with no queue)
 * without invoking/faking it. Human without managing governor falls back to
 * stockpile(-38SMAC/-69SMAX) only if current item still equals -facility and
 * waiting queue_size0. Current item is POST-notification state, not stale input
 * before real user choice. No notification acknowledgment, callback, phase,
 * World mutation, queue completion, AI production choice or UI is supplied.
 * Ordinary IDs2..33/64 and queue_size0..9 only; errors preserve output.
 */
LogicError completion_tail(const CompletionTailInput&,CompletionTailResult&);
}
#endif
