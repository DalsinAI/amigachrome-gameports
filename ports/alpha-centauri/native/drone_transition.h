/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker default mod_base_drones/mod_drone_riot restoration policy, MIT,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af. Not recovered original drone_riot.
 * Base-game ordinal adaptation: draw%39, destroy only2..33 before SkyHydro34;
 * pinned SMAX source draws%70 and accepts2..64 before SkyHydro65.
 */
#ifndef ALPHA_DRONE_TRANSITION_H
#define ALPHA_DRONE_TRANSITION_H
#include "combat_crt.h"
namespace alpha {
enum class DroneTransitionError {ok,bad_state,bad_buffer,unsupported,overflow,out_of_memory};
enum class DroneNotice {golden_started=1,golden_ended,riot_warning,riot_managed_start,riot_ended,riot_managed_end,facility_destroyed};
struct DroneTransitionFacts {
 int32_t population,talents,drones,superdrones,pending_police,assimilation_turns,owned_bases,difficulty,ranking;
 uint32_t state_flags,governor_flags;
 uint64_t built,protected_facilities;
 bool nerve_stapled,telepathic_matrix;
};
struct DroneTransitionResult {
 uint32_t state_flags,random_draws,facility_attempts;
 uint64_t built;
 int32_t destroyed_facility;
 DroneNotice notices[3];
 uint32_t notice_count;
 bool golden_age,rioting,recompute_required,repaint_required;
};
/* Fresh psych facts are required; no saved drone/talent totals qualify them.
 * Flags0x4/0x2 and exact notification branches, first-turn protection, ongoing
 * police/low-pop gates, free-facility protection and bounded damage attempts.
 * Thinker intentionally removes revolt; this policy does not fabricate it.
 * Separate shared game CRT state is explicit, never inferred from movement,
 * map or tick RNG. Only successfully consumed draws commit; all failures keep
 * facts/state/output unchanged, including partial aliases.
 */
DroneTransitionError drone_transition(const DroneTransitionFacts&,CombatRandomState&,DroneTransitionResult&);
}
#endif
