/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented original observed retirement-history wire contract.
 * OriginalPrototype layout from Thinker(MIT), see THINKER-LICENSE.txt.
 */
#ifndef ALPHA_RETIREMENT_HISTORY_H
#define ALPHA_RETIREMENT_HISTORY_H
#include "game_logic.h"
#include "original_game.h"
namespace alpha {
constexpr size_t retirement_history_bytes=8*64*8;
struct RetirementHistoryResult {int32_t next_slot;uint8_t table[retirement_history_bytes];};
/* Complete callback-free original retirement history append. Cursor is the
 * selected faction's next slot0..63. Stores ability flagsLE32 and original
 * chassis/weapon/armor/reactor bytes into that slot; advances modulo64. All
 * other table bytes retained. Exact4096-byte table, faction0..7, valid cursor
 * required. Output disjoint from complete table and prototype object; all
 * range/alias failures preserve output. No component normalization or naming.
 * This is a proposed history stage only: no live unit conversion/kill/salvage,
 * queue changes, full retirement, grant/discovery, World mutation or save
 * commitment. Matching current table/cursor/design are caller-owned facts.
 */
LogicError retirement_history_stage(const uint8_t*,size_t,int32_t faction,int32_t cursor,
 const OriginalPrototype&,RetirementHistoryResult&);
}
#endif
