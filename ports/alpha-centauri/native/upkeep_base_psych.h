/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker default mod_base_psych/adjust_psych/psych_check restoration policy,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af. See THINKER-LICENSE.txt.
 * Not recovered original base_psych: default diminishing returns and forced
 * talents are intentionally identified as Thinker policy, without mod knobs.
 */
#ifndef ALPHA_BASE_PSYCH_H
#define ALPHA_BASE_PSYCH_H
#include <stdint.h>
namespace alpha {
enum class UpkeepPsychError {ok,bad_state,bad_buffer,unsupported,out_of_memory,overflow};
struct PsychFacts {
 int32_t population,specialists,content_population,base_limit,base_index,faction_base_count;
 int32_t difficulty,assimilation_turns,rule_drone,rule_talent,no_drone_bonus;
 int32_t pending_talent,pending_police,fresh_psych,genejack_drones,pacifism_units;
 int32_t police_effects[3];
 bool genejack,recreation_commons,hologram_or_virtual,research_hospital,nanohospital;
 bool paradise_garden,planetary_transit,nerve_stapled,punishment_sphere,telepathic_matrix;
 bool longevity_vaccine,human_genome,clinical_immortality;
 int32_t pending_economics;
};
struct PsychResult {
 int32_t talents,drones,superdrones;
 int32_t efficiency_drones,capture_drones,police_total;
 int32_t rows[5][3];
 bool can_riot,will_riot;
};
/* Fresh psych energy, actual proposed specialist count and fresh support
 * pacifism are required. police_effects are three strongest eligible unit/
 * project values, strongest first, not saved police totals. No world mutation
 * or faction hypothetical social_psych aggregate is performed. Bounded ordinary
 * population0..127 and pending TALENT/POLICE[-100,100] domain. Error leaves output.
 */
UpkeepPsychError base_psych_policy(const PsychFacts&,PsychResult&);
}
#endif
