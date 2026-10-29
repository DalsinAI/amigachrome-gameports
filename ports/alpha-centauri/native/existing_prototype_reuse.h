/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently implemented observed original proposal early-return contract.
 * OriginalWorld layout from Thinker(MIT), see THINKER-LICENSE.txt.
 */
#ifndef ALPHA_EXISTING_PROTOTYPE_REUSE_H
#define ALPHA_EXISTING_PROTOTYPE_REUSE_H
#include "custom_prototype_construction.h"
namespace alpha {
enum class ProposalMode {unobserved,normal};
enum class ExpansionObservation {unobserved,disabled,enabled};
enum class ExistingProposalError {ok,bad_state,unsupported_runtime,unsupported_strength,allocation_required};
enum class ExistingProposalKind {retired,scout,duplicate,reused};
struct ExistingProposalRequest {int32_t faction,chassis,weapon,armor,reactor,plan;uint32_t abilities;ProposalMode mode;ExpansionObservation expansion;};
struct ExistingProposalResult {ExistingProposalKind kind;int32_t id;OriginalPrototype prototype;};
/* Complete normal-mode human original early-return domain only. Observations
 * are explicit input data, not session authority. History/scout/duplicate are
 * genuine unchanged refusals/returns; reuse proposes original obsolete-bit and
 * plan-2 prototyped changes. No naming/slot allocation, mutation or award.
 * Unsupported strength/runtime/allocation paths fail before output writes.
 * All owned/raw World, Rules, definitions and request ranges are disjoint from
 * result; failures preserve result and all inputs. */
ExistingProposalError world_existing_prototype_proposal(const OriginalWorld&,Variant,
 const Rules&,const UnitDefinitions&,const ExistingProposalRequest&,ExistingProposalResult&);
}
#endif
