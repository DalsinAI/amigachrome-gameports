/* SPDX-License-Identifier: GPL-3.0-or-later
 * Thinker map.cpp::is_sensor/base_find/vector_dist and veh_combat.cpp,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af; THINKER-LICENSE.txt.
 */
#ifndef ALPHA_WORLD_COMBAT_SENSORS_H
#define ALPHA_WORLD_COMBAT_SENSORS_H
#include "combat_rounds.h"
#include "original_world.h"
namespace alpha {
/* Read-only source-default sensor_defense_ocean=0 policy. Mask1 qualifies the
 * ordinary sensor bonus, mask2 its additional Geosync bonus. Scans original
 * 25 offsets, skips invalid flat/polar coordinates, wraps cylindrical x.
 * A raw sensor takes priority over a GSP at the same tile. Nearest GLOBAL base
 * uses source vector distance and highest-index tie winner. Land sensors need
 * neutral/defender territory; ocean sensor tiles need nearest base ownership.
 * The rules percentage is applied later, once per bit with ordered rounding.
 * No precomputed/saved strength or input/output mutation on rejection.
 */
CombatError world_combat_sensor_mask(const OriginalWorld&,int32_t defender,
 int32_t x,int32_t y,uint32_t&output);
}
#endif
