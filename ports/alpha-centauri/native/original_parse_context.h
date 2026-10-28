/* SPDX-License-Identifier: GPL-3.0-or-later. Independently reconstructed from
 * complete original parse_say/parse_says effects; no executable bytes included. */
#ifndef ALPHA_ORIGINAL_PARSE_CONTEXT_H
#define ALPHA_ORIGINAL_PARSE_CONTEXT_H
#include "discovery_reactor_stage.h"
namespace alpha {
struct OriginalParseContext {char strings[10][256];int32_t genders[10],plurals[10],default_gender,default_plural;};
struct OriginalParseTextInput {DiscoveryRuntimeObservation runtime;int32_t index,gender,plural;const uint8_t* bytes;size_t size;};
enum class OriginalParseError {ok,bad_state,runtime_required,bad_index,bad_text,overlong};
/* Complete bounded owned-text copy effects, common to both original editions.
 * Actual instructions use256-byte slots in BOTH editions. Negative gender/
 * plurality select the supplied current defaults (even negative defaults).
 * Copies through first NUL only, retaining every trailing byte and other slot.
 * No imported PC pointer or guessed StringManager fallback. Caller must supply
 * resolved literal bytes with NUL in the span, <=255 bytes before it. Negative
 * indices and original unbounded overflow paths reject; no unsafe emulation.
 * Every output/input range overlap or wrapped span rejects before writes.
 * No template expansion, file/popup/voice callback, UI or World authority.
 */
OriginalParseError original_parse_text(const OriginalParseContext&,const OriginalParseTextInput&,OriginalParseContext&);
}
#endif
