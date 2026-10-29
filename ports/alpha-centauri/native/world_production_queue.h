/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PRODUCTION_QUEUE_H
#define ALPHA_WORLD_PRODUCTION_QUEUE_H
#include "world_unit_availability.h"
#include "world_facility_options.h"
#include "queue_shift.h"
namespace alpha {
struct WorldProductionQueue {
 QueueShiftResult queue;uint32_t iterations;bool available,project_notification_required;
};
/* Read-only proposal for the complete structural/retry portion of base_queue.
 * Uses real current World unit/facility predicates (facility queue_count0), no
 * callback booleans. Copies base queue/owner unit counters privately. All retry
 * iterations preserve original byte-wrap and unused tail slots. No-base and
 * saved queue sentinel -1 reject. Faction1..7 from actual base owner.
 * Project audio/draw are reported obligations, not performed or faked. This
 * grants no session/phase authority and commits nothing. Caller must supply the
 * actual current post-completion World when used after facility installation;
 * this query does not install a facility or create a unit. Output aliases into
 * inputs/World storage reject before writes; any failure preserves output.
 */
LogicError world_production_queue(const OriginalWorld&,const Rules&,const UnitDefinitions&,
 const FacilityDefinitions&,Variant,bool expansion_enabled,size_t base,WorldProductionQueue&);
}
#endif
