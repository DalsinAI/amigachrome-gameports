/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_MOVEMENT_PRICE_H
#define AC_ALPHA_WORLD_MOVEMENT_PRICE_H
#include "world_unit_stats.h"
#include "terrain_price.h"
namespace alpha {
/* Price one adjacent original-map edge for a saved unit/design. Uses actual
 * faction Planet and Xenoempathy ownership. Pure price, not an action gate.
 * Requires matched original base/expansion component IDs and bounded project
 * references. Output remains unchanged on any failure.
 */
LogicError world_movement_price(const OriginalWorld& world,const UnitDefinitions& definitions,
 size_t vehicle,int32_t road_rate,int32_t x_to,int32_t y_to,bool ignore_rough,uint32_t& output);
}
#endif
