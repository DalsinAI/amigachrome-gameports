/* SPDX-License-Identifier: GPL-3.0-or-later. Original behavior independently
 * implemented using original observations and MIT Thinker record layouts. */
#ifndef ALPHA_HUMAN_COMPONENT_PRUNING_H
#define ALPHA_HUMAN_COMPONENT_PRUNING_H
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
enum class PruningRuntime {unknown,multiplayer_off,multiplayer_on};
enum class HumanPruningError {ok,bad_state,unsupported_runtime};
struct HumanPruningResult {uint16_t flags[512];size_t changed;};
/* COMPLETE prune_protos nonmodal domain: actual saved human membership, original
 * saved auto-prune preference OFF, and explicit observed MultiplayerActive OFF.
 * Runtime unknown/on rejects; a caller enum is NOT session/AI/action authority.
 * Does not infer multiplayer from a save or forge production-choice callbacks.
 * Returns owned flags for all512 designs, preserving every other World field.
 * Ordered component coverage may mark own active unprototyped designs0x104 and
 * newly prototyped designs participate in later own-index comparisons. Real
 * technology availability gates predefined models; own models require flag4.
 * Other human/AI/network/dialog pruning paths reject BEFORE writes. No World
 * commit, name/design allocation, prune callback, full proposal or award.
 * Full input/output overlap rejects and all errors preserve output.
 */
HumanPruningError world_human_component_pruning(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,PruningRuntime,int32_t faction,int32_t selected_id,HumanPruningResult&);
/* Same complete restricted human domain, virtualizing the genuine selected
 * postconstruction/name-store record before ordered coverage. No World write.
 * The selected candidate must be own/custom/active; all aliases reject. */
HumanPruningError world_human_component_pruning_candidate(const OriginalWorld&,Variant,const Rules&,
 const UnitDefinitions&,PruningRuntime,int32_t faction,int32_t selected_id,
 const OriginalPrototype& candidate,HumanPruningResult&);
}
#endif
