/* SPDX-License-Identifier: GPL-3.0-or-later. Independent bounded reconstruction
 * of the original parse_string resolved string/grammar effects. */
#ifndef ALPHA_ORIGINAL_TEMPLATE_H
#define ALPHA_ORIGINAL_TEMPLATE_H
#include "original_parse_context.h"
namespace alpha {
struct OriginalTemplateInput {DiscoveryRuntimeObservation runtime;int32_t language_mode;const uint8_t* bytes;size_t size;};
struct OriginalExpandedText {char bytes[8192];size_t length;};
enum class OriginalTemplateError {ok,bad_state,runtime_required,bad_text,unsupported,grammar_held,overlong};
/* String labels use the decimal slot suffix; $$ escapes $. Supports explicit
 * M/F/N or slot grammar selectors with original 1/2/3/5-colon selection rules,
 * including indirect slot alternatives. Copies owner bytes without UTF conversion.
 * Explicit observed language mode is data, not authority. French contraction
 * mode1, numeric/# selectors, links, malformed/nested directives and unsafe
 * original reads remain held. No popup/file/render/voice outcome, no World writes.
 * Result and every input remain unchanged on failure; all aliases reject.
 */
OriginalTemplateError original_expand_template(const OriginalParseContext&,const OriginalTemplateInput&,OriginalExpandedText&);
}
#endif
