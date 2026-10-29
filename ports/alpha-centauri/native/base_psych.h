/* SPDX-License-Identifier: GPL-3.0-or-later. Algorithm provenance:
 * OpenSMACX, Brendan Casey, 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Its WIP psych routine is incomplete; original edition behavior checked
 * privately against complete original helpers, including all five rows.
 */
#ifndef ALPHA_BASE_PSYCH_H
#define ALPHA_BASE_PSYCH_H
#include <stdint.h>
#include <stddef.h>
namespace alpha {
enum class PsychError : uint32_t { ok, bad_state, bad_buffer, overflow };
struct PoliceUnitFacts {
 int32_t prototype_id,plan,weapon_offense,triad;
 uint32_t double_police;
};
struct BasePsychFacts {
 uint32_t expansion,human,upkeep_stage;
 int32_t population,specialists,assimilation_turns,difficulty,pending_efficiency;
 int32_t map_factor,base_id,faction_base_count,rule_drone,rule_talent;
 uint32_t has_nodrone;
 int32_t nodrone,pending_talent,pending_police,pacifism_count,psych;
 int32_t genejack_drones,pending_economics,nerve_staple_turns;
 uint32_t facilities[2],owned_projects,worm_police;
 const PoliceUnitFacts* police_units;
 size_t police_count;
 int32_t social_psych[82];
};
struct BasePsychResult {
 int32_t rows[5][3]; // talent, drone, superdrone at each original phase
 int32_t talents,drones,superdrones,social_psych[82];
};
/* Explicit current facts only. Physical facility bits and original PROJECT
 * indices; no free-facility or queue interpretation here. The ordered unit
 * list is the actual garrison stack, including allies; double_police must be
 * resolved using edition-correct ability rules before this call. Maximum2048.
 * Stage1 proposes the72 original social projections; all82 input slots are
 * retained otherwise. SMAC ignores NODRONE/WORMPOLICE/BroodPit, as originals.
 * Population0..127, specialists0..population, rule divisors>=0, explicit
 * mapfactor>=0/baseid0..511/basecount0..512. All executed signed32 arithmetic
 * checked. Errors/overlapping buffers preserve all output and inputs.
 * No World, authority, actor mutation/stack repair, accounting commit, turn,
 * UI, filesystem, or player-save behavior.
 */
PsychError base_psych(const BasePsychFacts&,BasePsychResult&);
}
#endif
