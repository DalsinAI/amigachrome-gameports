/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-state adaptation of OpenSMACX veh.cpp, Brendan Casey (C) 2013-2021,
 * pinned 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_PROTOTYPE_H
#define AC_ALPHA_PROTOTYPE_H
#include "game_logic.h"
namespace alpha {
struct PredefinedPrototype {
  char name[32];
  int32_t chassis, weapon, armor, reactor, prerequisite;
  uint32_t abilities;
  int32_t cost, carry, plan, role_code, icon, obsolete_factions, combat_factions;
  uint32_t flags, speed_points;
};
/* Qualified definition tables required. Pure component pricing, not prototype
 * history or faction build availability. Failure leaves cost unchanged.
 */
LogicError calculate_prototype_cost(const UnitDefinitions& definitions, uint32_t chassis,
                                    uint32_t weapon, uint32_t armor, uint32_t abilities,
                                    uint32_t reactor, uint32_t& cost);
/* Only predefined IDs0..63. Applies make_proto's basic-ID path, blueprint
 * overrides, transport_val and speed_proto. No custom designer history,
 * faction-granted abilities/projects, unit damage/morale or movement occurs.
 * Rejects source arithmetic/storage overflows instead of wrapping. Failure
 * leaves output unchanged. Road rate must be qualified1..100.
 */
LogicError derive_predefined_prototype(const UnitDefinitions& definitions, size_t id,
                                       int32_t road_rate, PredefinedPrototype& output);
}
#endif
