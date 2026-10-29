/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_NODE_ARTIFACTS_H
#define ALPHA_NODE_ARTIFACTS_H
#include "original_world.h"
#include "game_logic.h"
namespace alpha {
enum class NodeRuntime { owned_offline,network };
struct NodeArtifacts {uint32_t state_flags,visited,artifacts,changed_orders;uint8_t clear_order[4096];};
/* Read-only Network Node post-install effect proposal. The physical Node must
 * already exist. Explicit owned_offline runtime only: not inferred from save
 * flags or a claim about an original running game's network globals. Coherent
 * reciprocal stacks and vehicle-present map bits required; no repair/logging
 * fallback or DirectPlay stack_fix branch is simulated. Clears base flags0xc00
 * and proposes order0 for every plan12 artifact in the actual base tile stack,
 * including other factions exactly as the original branch (no ownership test).
 * Mask includes already-order0 artifacts; changed_orders counts real changes.
 * No World mutation, mineral/queue/research effect, complete facility action,
 * phase authority, turn or frontend input. Errors preserve output; aliases into
 * owned World storage reject. At most32767 actors (4096byte result bitmap).
 */
LogicError node_artifacts(const OriginalWorld&,Variant,NodeRuntime,size_t base,NodeArtifacts&);
}
#endif
