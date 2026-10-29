/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented observed original collision-comparison contract.
 */
#ifndef ALPHA_PROTOTYPE_PREFIX_CONTEXT_H
#define ALPHA_PROTOTYPE_PREFIX_CONTEXT_H
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
enum class PrototypePrefixObservation {unobserved,supplied_bytes};
struct PrototypePrefixFact {PrototypePrefixObservation observation;uint8_t bytes[52];};
enum class PrototypePrefixError {ok,bad_state,runtime_context_required};
struct PrototypePrefixComparison {int32_t sign;uint32_t logicalBytesCompared;bool fromOwnedHistory;};
/* Complete bounded alternate-name comparison after original scan index becomes
 * -1. Base prefix is the last52 CURRENT owned bunglist bytes; SMAX requires an
 * explicit supplied52-byte runtime fact (data, never authority). The prefix is
 * followed by actual owned Prototype0.name[32]. Term includes exactly one final
 * NUL,1..256bytes. Counts logical byte comparisons, not x86 wide-load traffic.
 * No suffix/name choice/allocation/award or World mutation. Errors preserve out;
 * output disjoint from all owned/raw World ranges, fact and term range. */
PrototypePrefixError world_prototype_prefix_compare(const OriginalWorld&,Variant,
 const PrototypePrefixFact&,const uint8_t* term,size_t length,PrototypePrefixComparison&);
}
#endif
