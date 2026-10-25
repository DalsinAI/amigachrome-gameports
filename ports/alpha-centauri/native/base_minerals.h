/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-state adaptation of OpenSMACX base.cpp::base_minerals,
 * Brendan Casey (C)2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_BASE_MINERALS_H
#define ALPHA_BASE_MINERALS_H
#include "game_logic.h"
namespace alpha {
struct MineralInput {
 int32_t intake,convoy_to,convoy_from,maintenance,raw_eco_damage,population;
 int32_t clean_minerals_modifier,satellites,atrocities,tectonic_detonations;
 int32_t sea_level,sea_level_rises,tech_ranking,transcendent_techs,planet,native_life;
 int32_t faction_difficulty,global_difficulty,queue_id;
 bool expansion; // Explicit runtime edition; queue IDs differ between SMAC and SMAX.
 bool human,ascending,transcendence_enabled,perihelion;
 bool quantum_converter,robotic_assembly,genejack,nanoreplicator;
 bool singularity_inductor,bulk_matter,centauri_preserve,temple,pholus_mutagen,space_elevator;
};
struct MineralResult {
 int32_t intake,consumption,surplus,final_surplus,eco_damage,bonus,inefficiency;
};
/* Bounded original mineral/ecology stage (native qualification pending), not production completion/turns.
 * Inputs are PRE-STAGE intake and terraforming eco damage, not the saved
 * post-calculation intake_2/eco_damage. Convoy/support/yield/ascending facts
 * must be resolved by their own source-backed stages, never fake callbacks.
 * Original convoy multiplication, Nanoreplicator eco reduction and the
 * pre-Space-Elevator final_surplus are intentional (not Thinker fixes).
 * Runtime edition is explicit: orbital facility IDs34..37 in base,65..68 in
 * expansion. Nonzero tectonic counts are unsupported in base mode.
 * Original instruction checks also correct the recovered source's unsigned
 * types and combined atrocity/tectonic rounding. Only nonnegative resource/count
 * inputs and representable original signed32 arithmetic are supported.
 * Reject overflow before replacing output.
 */
LogicError calculate_minerals(const MineralInput&,MineralResult&);
}
#endif
