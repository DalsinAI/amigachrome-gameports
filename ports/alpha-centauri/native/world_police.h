/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original has_abil/veh_at traversal adapted from OpenSMACX veh.cpp,
 * Brendan Casey (C)2013-2021, pinned2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_WORLD_POLICE_H
#define ALPHA_WORLD_POLICE_H
#include "world_unit_stats.h"
#include "base_psych.h"
namespace alpha {
enum class PoliceError { ok,bad_state,bad_buffer,out_of_memory };
enum class PoliceSnapshotProfile { offline_validated=0 };
struct WorldPoliceGarrison {
 Variant variant;int32_t base;uint32_t count;
 int32_t vehicle_ids[2048];PoliceUnitFacts units[2048];
};
/* Scope exactly double-police bit22. SMAC raw-only; SMAX owner=id/64 grants
 * require actual technology. Raw/predefined shortcuts precede grant reads.
 * Every reached FREEABIL slot is checked in original order, even another
 * ability's slot; successful matching grant short-circuits later slots.
 * No Deep Radar/Nethack/general-ability claim. Errors/aliases preserve output.
 */
PoliceError world_double_police(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 size_t prototype,bool&);
/* Explicit offline snapshot query, not inferred live network/modal authority.
 * Validates tile bit and complete reciprocal same-coordinate single-root stack,
 * rejects corrupt stacks instead of original editor/network repair callbacks.
 * Includes allies and noncombat units in actual head-to-tail order. Reads only
 * plan/chassis/weapon/double-police dependencies; no reactor/speed requirement.
 * Caller owns the fixed output; transactional temporary allocation may fail.
 * Entire input/output ranges are disjoint, including all World allocations.
 * No stack repair, actor/accounting mutation, psych execution, turn or UI.
 */
PoliceError world_police_garrison(const OriginalWorld&,const UnitDefinitions&,const Rules&,
 size_t base,PoliceSnapshotProfile,WorldPoliceGarrison&);
}
#endif
