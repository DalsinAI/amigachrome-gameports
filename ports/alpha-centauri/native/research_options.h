/* SPDX-License-Identifier: GPL-3.0-or-later
 * Recovered eligibility: OpenSMACX technology.cpp / Thinker tech.cpp.
 * Brendan Casey (C)2013-2021; source pins documented in RESEARCH-OPTIONS.md.
 */
#ifndef ALPHA_RESEARCH_OPTIONS_H
#define ALPHA_RESEARCH_OPTIONS_H
#include "world_unit_stats.h"
namespace alpha {
enum class ResearchAvailability : uint8_t { available,already_known,disabled,base_mode_excluded,missing_prerequisite,planet };
struct ResearchTechnology {
 ResearchAvailability availability;
 uint8_t missing_prerequisites;
 bool known,repeatable;
};
struct ResearchOptions {
 ResearchTechnology technologies[technology_count];
 size_t known_count,available_count;
 /* Saved snapshot values only. No recalculation, selection or awarding. */
 int32_t current_id,accumulated,cost,transcendent_count;
 bool current_id_supported;
};
/* Explicit runtime game variant is required: data_version is a wire-layout tag,
 * not the original ExpansionEnabled runtime flag. ID slots retain original
 * meanings (78..86 expansion-only,88 repeatable Transcendent Thought).
 * Requires qualified same-profile Rules; malformed references fail before any
 * output replacement. The owned world and runtime seed remain unchanged.
 */
LogicError world_research_options(const OriginalWorld&,const Rules&,Variant,int32_t faction,ResearchOptions&);
}
#endif
