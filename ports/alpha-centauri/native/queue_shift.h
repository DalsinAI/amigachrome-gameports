/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original queue shift independently adapted from Thinker base.cpp (MIT),
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af; see THINKER-LICENSE.txt.
 */
#ifndef ALPHA_QUEUE_SHIFT_H
#define ALPHA_QUEUE_SHIFT_H
#include "game_logic.h"
namespace alpha {
struct QueueShiftInput { int32_t queue_size,items[10]; uint8_t units_queue[512]; };
struct QueueShiftResult { int32_t queue_size,items[10]; uint8_t units_queue[512]; bool shifted; };
/* One structural iteration BEFORE actual availability predicates. No candidate
 * is declared buildable. queue_size is the count AFTER the current item (0..9),
 * not the total occupied slots. For positive size decrement old unit counter
 * modulo256 if item0>=0, copy items[1..size] left and decrement size. Retain
 * untouched tail slots exactly, including the duplicate last shifted slot.
 * Size0 is unchanged. Old unit IDs>=512 reject before writes; negative IDs are
 * opaque here and never negated. No World/session mutation, queue retry loop,
 * eligibility callbacks, project audio/draw, completion, phase or player input.
 * Failure preserves output. Input and output can refer to separate values only.
 */
LogicError queue_shift(const QueueShiftInput&,QueueShiftResult&);
}
#endif
