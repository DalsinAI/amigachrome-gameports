/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently adapted from Thinker monument.cpp (MIT, THINKER-LICENSE.txt),
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af; original edition differences retained.
 */
#ifndef ALPHA_MONUMENT_EFFECTS_H
#define ALPHA_MONUMENT_EFFECTS_H
#include "game_logic.h"
#include <stddef.h>
namespace alpha {
constexpr size_t monument_record_bytes=0x4f4,monument_table_bytes=0x27a0;
struct MonumentEffectInput {
 Variant variant;const uint8_t* monuments;size_t size;
 int32_t faction,local_faction,event,year;uint32_t faction_rule_flags;
 uint8_t facility_base_name[24];
};
struct MonumentEffectResult {
 uint8_t record[monument_record_bytes];bool changed,presentation_required;
};
/* Pure proposal for event4 (first facility; supplied name is the BASE name)
 * or12 (first Preserve); event2 is the Secrets history stage. Input is the entire eight-record original LE table.
 * Output preserves every unknown byte of the selected record. Name copying
 * follows strncpy(...,24), including zero padding and nonterminated full names.
 * Existing nonzero achieved/first markers retain original truthiness; last
 * qualifying other first-marker wins. Only SMAX suppresses alien rule flag0x80.
 * Year is explicit current game_year, not a guessed save/footer timestamp.
 * presentation_required reports the original local-player monument(1) CALL;
 * preferences/scenario/end-state suppression belongs to that unimplemented
 * presentation lifecycle. No fake acknowledgment, live World mutation, save
 * encoder, completion authority, game turn or modal window is supplied.
 * Exact input size, faction/local0..7, event2/4/12 required. Errors (including
 * input/output overlap) leave output unchanged; no allocation/native wire dumps.
 */
/* Secrets: existing nonzero achieved updates param2 then param1 only when -1;
 * once both populated it still writes latest_event_id=2 and requests local
 * presentation. changed reports actual record changes for this event, so a
 * presentation request can be true when changed is false. No tech ownership,
 * first-Secrets award flag, bonus advance or discovery transaction is supplied.
 */
LogicError monument_effect(const MonumentEffectInput&,MonumentEffectResult&);
}
#endif
