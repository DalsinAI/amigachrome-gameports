/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independent owned stages from original propose_proto record-write observations.
 * See PROPOSER-RECORD-STAGES.md; not the complete proposer or a discovery award.
 */
#ifndef ALPHA_PROPOSER_RECORD_STAGES_H
#define ALPHA_PROPOSER_RECORD_STAGES_H
#include "original_world.h"
#include "automatic_prototype_name.h"
namespace alpha {
enum class ProposerRecordError {ok,bad_state};
struct ProposerPreparationFacts {int32_t requested_group;int32_t difficulty;};
/* prior is the genuine pre-retirement record; constructed is the genuine
 * post-retirement make_proto result. Distinct phases must not be conflated.
 * Facts are observations/data, never authority. Full signed group argument is
 * retained: nonnegative stores its low byte, only exact10 sets plan3.
 * All input/output ranges must be disjoint; errors leave output unchanged.
 */
ProposerRecordError prepare_proposer_record(const OriginalPrototype& prior,
 const OriginalPrototype& constructed,const ProposerPreparationFacts&,OriginalPrototype&);
/* Complete automatic-name store slice: strncpy32, NUL31, clear flag2.
 * Requires a terminated bounded qualified logical name, not an invented chooser
 * result. Other fields remain unchanged. Does not perform naming/allocation.
 */
ProposerRecordError store_proposer_automatic_name(const OriginalPrototype& prepared,
 const PrototypeLogicalName&,OriginalPrototype&);
/* Read-only owned World bindings. These produce candidates only: no allocation,
 * retirement, pruning, production remap, sharing, mutation or authority grant.
 * The World is the pre-retirement snapshot for preparation. Constructed record
 * provenance and mandatory surrounding effects remain the caller's obligation.
 */
ProposerRecordError world_prepare_proposer_record(const OriginalWorld&,Variant,int32_t id,
 const OriginalPrototype& constructed,const ProposerPreparationFacts&,OriginalPrototype&);
ProposerRecordError world_store_proposer_automatic_name(const OriginalWorld&,Variant,
 const OriginalPrototype& prepared,const PrototypeLogicalName&,OriginalPrototype&);
}
#endif
