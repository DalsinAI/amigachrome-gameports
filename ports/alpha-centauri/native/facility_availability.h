/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-context adaptation of OpenSMACX base.cpp::facility_avail (Brendan Casey,
 * C2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c), distinguishing
 * original instruction-backed branches from the documented later bug fixes.
 */
#ifndef ALPHA_FACILITY_AVAILABILITY_H
#define ALPHA_FACILITY_AVAILABILITY_H
#include "game_logic.h"
namespace alpha {
struct FacilityAvailabilityContext {
 Variant profile;
 bool expansion_enabled,caretaker,has_base,coastal;
 int32_t faction,difficulty,queue_count;
 uint32_t game_rules,faction_rule_flags;
 uint8_t facilities[12],known_technology[89];
 int8_t project_owner[64]; // -1unbuilt,-2destroyed,0..7built owner
 int32_t queue[10];
};
/* Pure availability predicate, not production completion/queue placement/turn.
 * Explicit qualified facts; coastal is the original adjacent-ocean predicate,
 * not terrain guessed from a screenshot. No implicit authority or wire binding.
 * Base and SMAX ordinal tables differ; SMAX runtime expansion flag is separate.
 * QueueCount0..10, faction1..7,difficulty0..5; malformed facts reject without
 * replacing output. Requires a successfully parsed matching facility table.
 */
LogicError facility_available(const FacilityDefinitions&,const FacilityAvailabilityContext&,
 size_t facility,bool& available);
}
#endif
