/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_caller_order.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
TerrainCallerStatus plan_world_terrain_order(const OriginalWorld&w,int x,int y,const TerrainObservationRuntime&v,const TerrainCallerRuntime&c,TerrainCallerOrder&out){
 using S=TerrainCallerStatus;if(!v.supplied||!c.supplied)return S::missing_runtime;const auto&m=w.game().map.info;if(!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||v.viewer< -1||v.viewer>7||(v.edition!=TerrainSourceEdition::smac&&v.edition!=TerrainSourceEdition::smacx)||c.minimum_light< -256||c.maximum_light>256||c.minimum_light>c.maximum_light||alias(w,&out,sizeof out)||overlap(&out,sizeof out,&v,sizeof v)||overlap(&out,sizeof out,&c,sizeof c))return S::invalid_input;
 const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);bool land=(t->climate>>5)>=3;TerrainCallerOrder result{};result.land_clamp_before_observation=land;result.pass_minimum_light=land?-4:c.minimum_light;result.pass_maximum_light=land?4:c.maximum_light;result.exit_minimum_light=-256;result.exit_maximum_light=256;
 const bool outline=c.outline_parameter>21||(!(v.game_rules&0x100u)&&v.viewer>=0&&!(w.faction(v.viewer)->player_flags&0x200u)&&!(t->visibility&(1u<<v.viewer)));
 if(outline)result.path=TerrainCallerPath::outline_only;
 else if(v.draw_flags&0x10000000u)result.path=TerrainCallerPath::terrain_hidden;
 else if(v.draw_flags&~uint32_t(1057))return S::held_draw_mode;
 else result.path=land?TerrainCallerPath::land_then_coast:TerrainCallerPath::water;
 out=result;return S::resolved;
}
}
