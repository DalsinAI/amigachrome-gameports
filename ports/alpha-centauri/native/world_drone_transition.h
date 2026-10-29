/* SPDX-License-Identifier: GPL-3.0-or-later. See drone_transition.h. */
#ifndef ALPHA_WORLD_DRONE_TRANSITION_H
#define ALPHA_WORLD_DRONE_TRANSITION_H
#include "drone_transition.h"
#include "world_base_psych.h"
#include "world_energy_allocation.h"
namespace alpha {
struct WorldDroneStage {ProposedCitizenAssignment assignment;int32_t fresh_psych;};
struct WorldDroneResult {WorldPsychResult psych;DroneTransitionFacts facts;DroneTransitionResult transition;};
/* Fresh psych is recomputed from the same proposed current-population
 * assignment/count and its authoritative current-stage psych allocation.
 * Physical tile legality and specialist economic availability are prerequisites
 * of the qualified worker/allocation producer, not supplied by saved displays.
 * Derives actual owner count, pending police, flags, physical HQ, assimilation,
 * projects, ranking/difficulty and faction free-facility/technology protection.
 * Base-game humans only. No world/UI/event execution or original revolt claim;
 * proposes complete bounded flag/damage/notification state and separate CRT.
 * Every input/output/state buffer disjoint. Any error leaves state/output and
 * all world/rules/assignment inputs unchanged.
 */
DroneTransitionError world_drone_transition(const OriginalWorld&,const Rules&,const UnitDefinitions&,
 const BasicRules&,const FacilityDefinitions&,size_t base,const WorldDroneStage&,CombatRandomState&,WorldDroneResult&);
}
#endif
