/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-context adaptation of Brendan Casey's OpenSMACX (C) 2013-2021,
 * base.cpp::base_minerals, pin 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Original routine offset 004E9CB0. No original executable is required.
 */
#ifndef ALPHA_BASE_MINERALS_H
#define ALPHA_BASE_MINERALS_H
#include <stdint.h>
namespace alpha {
enum class MineralError {ok,bad_state,bad_buffer,overflow};
struct MineralFacts {
 int32_t fresh_intake,convoy_to,forces_maintenance,convoy_from;
 int32_t raw_eco_damage,planet_ecology,satellites,population;
 int32_t major_atrocities,tectonic_detonations,sea_level,sea_level_rises;
 int32_t faction_difficulty,game_difficulty,tech_ranking,transcendent_techs;
 int32_t planet_pending,native_life;
 bool quantum_converter,robotic_assembly,genejack,nanoreplicator;
 bool bulk_matter,singularity_inductor,centauri_preserve,temple,pholus;
 bool human,ascending_transcendence,perihelion,space_elevator_orbital;
};
struct UpkeepMineralResult {
 int32_t intake,consumption,surplus,surplus_final,eco_damage;
};
/* A pure accounting leaf, not a turn, production completion or support engine.
 * All inputs are fresh current-stage facts; support cost and convoys must be
 * computed by their real engines. Physical facilities and project effects are
 * separate; queued facilities grant no bonus. The original convoy multiplication
 * and Nanoreplicator ecology branch are preserved, not Thinker's later fixes.
 * Orbital doubling follows ecology and leaves surplus_final undoubled.
 * Normal nonnegative mineral/support/atrocity domains only. Each original signed
 * intermediate must fit int32; invalid/overflow/aliased inputs preserve output.
 */
MineralError base_minerals(const MineralFacts&,UpkeepMineralResult&);
}
#endif
