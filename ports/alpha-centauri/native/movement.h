/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned map view and predefined movement pricing adapted from OpenSMACX map.cpp
 * and veh.cpp::hex_cost, Brendan Casey (C)2013-2021,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef AC_ALPHA_MOVEMENT_H
#define AC_ALPHA_MOVEMENT_H
#include "prototype.h"
namespace alpha {
struct MapGeometry { int32_t width,height;bool flat; };
struct MovementTile {
  uint32_t items;
  uint8_t altitude,rockiness; // original encoded altitude/rock bits, not levels
};
struct MovementMap { MapGeometry geometry;const MovementTile* tiles;size_t count; };
struct MovementContext {
  int32_t faction,planet,road_rate;
  bool xenoempathy_dome,ignore_rough;
};
/* Width is even; coordinates use parity x+y even and original (x+width*y)/2.
 * Numeric coordinate domain is the original nonnegative signed16 range.
 * Horizontal wrap is exactly one source xrange step, not arbitrary modulo.
 */
LogicError movement_tile_index(const MapGeometry& geometry,int32_t x,int32_t y,size_t& index);
LogicError movement_wrap_x(const MapGeometry& geometry,int32_t x,int32_t& wrapped);
LogicError movement_distance_x(const MapGeometry& geometry,int32_t a,int32_t b,int32_t& distance);
/* Price an adjacent step using qualified predefined definitions. This is NOT
 * passability, occupancy, combat, ZOC, movement expenditure or turn dispatch.
 * Terrain access and action legality must be checked by those later layers.
 * All outputs remain unchanged on failure. No global game addresses/callbacks.
 */
LogicError adjacent_movement_cost(const MovementMap& map,const UnitDefinitions& definitions,
                                  size_t prototype,const MovementContext& context,
                                  int32_t x_from,int32_t y_from,int32_t x_to,int32_t y_to,
                                  uint32_t& cost);
}
#endif
