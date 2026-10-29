/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently adapted from Thinker veh.cpp (MIT, THINKER-LICENSE.txt),
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af; original edition branches checked.
 */
#ifndef ALPHA_UNIT_AVAILABILITY_H
#define ALPHA_UNIT_AVAILABILITY_H
#include "game_logic.h"
namespace alpha {
struct UnitAvailabilityInput {
 Variant variant;int32_t unit,faction;uint16_t flags;uint8_t obsolete;
 int8_t plan;uint8_t armor,weapon;uint32_t abilities,game_rules;
 bool expansion_enabled,base_present;uint8_t triad;bool coastal;
 int16_t technology;int32_t prerequisite1,prerequisite2;uint8_t technology_owners;
};
/* Pure admission from explicit qualified current facts, NOT a World binding,
 * owning queue loop, production order/completion/turn or player input. For
 * predefined IDs normalize actual prerequisite sentinels/ownership here.
 * SMAC ignores expansion_enabled and expansion-only component filters. SMAX
 * applies them when its explicit runtime expansion_enabled is false. A supplied
 * coastal fact is not original world geometry execution. Failure preserves out.
 */
LogicError unit_availability(const UnitAvailabilityInput&,bool& out);
}
#endif
