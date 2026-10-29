/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original monument entry adapted independently from Thinker monument.cpp,
 * MIT, THINKER-LICENSE.txt, dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef ALPHA_MONUMENT_PRESENTATION_H
#define ALPHA_MONUMENT_PRESENTATION_H
#include "monument_effects.h"
namespace alpha {
struct MonumentPresentationInput {
 Variant variant;const uint8_t* local_record;size_t size;
 uint32_t local_faction_rule_flags,more_preferences,game_state;
 int32_t multiplayer_active;bool new_event;
};
struct MonumentPresentationResult {
 uint32_t achieved_count;bool count_written,initialization_required;
};
/* Original monument entry ONLY, through counting and decision to call window
 * initialization. Uses the actual local faction's 0x4f4-byte LE record.
 * SMAX alien flag0x80 suppresses both manual and new-event entry; SMAC does not.
 * Any nonzero multiplayer suppresses. Manual entry ignores preference/scenario/
 * game-done gates. New events require preference0x1000000 clear, scenario0x10
 * clear, and not BOTH game-done0x1/final-score-done0x8.
 * Counts raw nonzero achieved markers ONLY for events0..14, not15..20.
 * count_written=false means original count global remains unchanged; returned
 * zero is not authority to clear it. Initialization may still fail; this API
 * never claims a window opened, requests no fake acknowledgment, performs no
 * modal draw/audio/callback/World mutation and advances no turn.
 * Exact size and disjoint output required; all errors preserve output.
 */
LogicError monument_presentation(const MonumentPresentationInput&,MonumentPresentationResult&);
}
#endif
