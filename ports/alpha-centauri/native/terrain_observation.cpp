/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_observation.h"
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
TerrainObservationStatus observe_world_terrain(const OriginalWorld&w,int x,int y,const TerrainObservationRuntime&c,TerrainObservation&out){
 using S=TerrainObservationStatus;const auto&m=w.game().map.info;if(!c.supplied)return S::missing_runtime;if(!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||c.viewer< -1||c.viewer>7||c.current_faction<0||c.current_faction>7||c.source_zoom< -16||c.source_zoom>16||(c.edition!=TerrainSourceEdition::smac&&c.edition!=TerrainSourceEdition::smacx)||alias(w,&out,sizeof out)||overlap(&out,sizeof out,&c,sizeof c))return S::invalid_input;
 const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);const uint32_t flags=c.viewer<0?0:w.faction(c.viewer)->player_flags;TerrainObservation result{};result.rainfall=(t->climate>>3)&3;result.rockiness=t->val3>>6;result.source_faction=(uint8_t)(c.viewer<0?((t->val3&7)?t->val3&7:c.current_faction):c.viewer);result.displayed_items=c.viewer<0||(flags&0x200)?t->items:c.viewer==0?t->landmarks:t->visible_items[c.viewer-1];result.far_profile=c.source_zoom< -6||(c.draw_flags&0x80000000u);
 if(c.draw_flags&0x1000000u){result.rainfall=result.rockiness=0;result.displayed_items=0;result.concealed=true;result.visible=true;}
 else{
  if((c.game_rules&0x100u)&&!(c.game_state&0x80u)){
   if(c.viewer<0)return S::held_negative_faction_lookup;
   if(!(flags&0x200u)&&!(t->visibility&(1u<<c.viewer))){result.rainfall=result.rockiness=0;result.displayed_items&=~0x20u;result.concealed=true;result.visible=false;result.landmarks_after=t->landmarks|0x200000u;out=result;return S::resolved;}
  }
  if((c.game_state&0x80u)||c.viewer<1||!(c.more_preferences&1u)||c.source_zoom< -7)result.visible=true;
  else return S::held_neighbour_visibility;
 }
 result.landmarks_after=t->landmarks&~0x200000u;out=result;return S::resolved;
}
}
