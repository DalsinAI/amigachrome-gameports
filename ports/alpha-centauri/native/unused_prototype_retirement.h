/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UNUSED_PROTOTYPE_RETIREMENT_H
#define ALPHA_UNUSED_PROTOTYPE_RETIREMENT_H
#include "original_world.h"
#include "retirement_history.h"
#include "prototype_retirement_queue.h"
namespace alpha {
enum class UnusedRetirementError {ok,bad_state,live_unit_effects_required,out_of_memory};
struct UnusedRetirementProposal {
 RetirementHistoryResult history{};RetirementQueueProposal queues;
 int32_t faction=-1,prototype_id=-1;bool ready=false;
};
/* Complete original retire_proto gameplay effects in the checked custom-design
 * NO LIVE REFERENCE domain. Actual owned World supplies history, cursor, design,
 * all bases and vehicle membership; owner must equal designID/64, factions1..7.
 * Any referencing unit rejects BEFORE writes, retaining kill/conversion/stack/
 * salvage obligations for broader domains. No modal/network callback is forged.
 * This read-only proposal has no revision authority, World write or save commit,
 * and is not pruning/proposal/reactor/discovery completion. Imported bunglist
 * remains the current immutable snapshot: future mutators must bind new owned
 * history rather than reuse stale baseline bytes. Result disjoint from entire
 * World/decoded/raw storage. Failures preserve prior result.
 */
UnusedRetirementError world_unused_prototype_retirement(const OriginalWorld&,Variant,int32_t faction,int32_t id,UnusedRetirementProposal&);
}
#endif
