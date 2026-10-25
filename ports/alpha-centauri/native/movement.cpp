/* SPDX-License-Identifier: GPL-3.0-or-later. See movement.h for attribution. */
#include "movement.h"
#include "terrain_price.h"
namespace alpha {
static bool geometry_valid(const MapGeometry& g) {
  return g.width>=2 && g.width<=32766 && !(g.width&1) && g.height>=1 && g.height<=32767;
}
LogicError movement_tile_index(const MapGeometry& g,int32_t x,int32_t y,size_t& index) {
  if(!geometry_valid(g) || x<0 || x>=g.width || y<0 || y>=g.height || ((x+y)&1))return LogicError::bad_state;
  index=((size_t)y*(size_t)g.width+(size_t)x)/2;return LogicError::ok;
}
LogicError movement_wrap_x(const MapGeometry& g,int32_t x,int32_t& wrapped) {
  if(!geometry_valid(g))return LogicError::bad_state;
  if(!g.flat) { if(x>=g.width)x-=g.width;else if(x<0)x+=g.width; }
  wrapped=x;return LogicError::ok;
}
LogicError movement_distance_x(const MapGeometry& g,int32_t a,int32_t b,int32_t& distance) {
  if(!geometry_valid(g) || a<0 || a>=g.width || b<0 || b>=g.width)return LogicError::bad_state;
  int32_t d=a>b?a-b:b-a;
  if(!g.flat && d>g.width/2)d=g.width-d;
  distance=d;return LogicError::ok;
}
static bool tile_valid(const MovementTile& tile) {
  return !(tile.altitude&31u) && (tile.rockiness==0 || tile.rockiness==0x40 || tile.rockiness==0x80);
}
LogicError adjacent_movement_cost(const MovementMap& map,const UnitDefinitions& definitions,
                                  size_t id,const MovementContext& ctx,
                                  int32_t sx,int32_t sy,int32_t dx,int32_t dy,uint32_t& output) {
  size_t source,dest;
  if(movement_tile_index(map.geometry,sx,sy,source)!=LogicError::ok ||
      movement_tile_index(map.geometry,dx,dy,dest)!=LogicError::ok || !map.tiles ||
      map.count<(size_t)(map.geometry.width/2)*(size_t)map.geometry.height ||
      source>=map.count || dest>=map.count || ctx.faction<0 || ctx.faction>7)return LogicError::bad_state;
  int32_t xdistance;
  if(movement_distance_x(map.geometry,sx,dx,xdistance)!=LogicError::ok)return LogicError::bad_state;
  const int32_t ydistance=sy>dy?sy-dy:dy-sy;
  if(!((xdistance==1 && ydistance==1) || (xdistance==2 && !ydistance) || (!xdistance && ydistance==2)))return LogicError::bad_state;
  const auto& from=map.tiles[source];const auto& to=map.tiles[dest];
  if(!tile_valid(from) || !tile_valid(to))return LogicError::bad_state;
  PredefinedPrototype p;
  auto error=derive_predefined_prototype(definitions,id,ctx.road_rate,p);
  if(error!=LogicError::ok)return error;
  const MovementDesign design{(uint32_t)id,p.abilities,p.speed_points,p.chassis,definitions.chassis[p.chassis].triad,
    definitions.weapons[p.weapon].offense,p.plan,id==14 || id==9,id==8 || id==15};
  // Keep the predefined-only API's existing native-ID and strict tile contract.
  return terrain_step_price(from,to,xdistance==1 && ydistance==1,design,ctx,output);
}
}
