/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_scene.h"
#include "terrain_world_masks.h"
#include "terrain_feature_layers.h"
#include "terrain_roads.h"
#include "terrain_rivers.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
TerrainSceneStatus render_world_ordinary_scene(const OriginalWorld&w,const Rules&r,const UnitDefinitions&d,int x,int y,const TerrainSceneRuntime&in,const TerrainCoastMetadata&meta,const TerrainCoastResources&coast,const TerrainOceanLightTable&ot,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 using S=TerrainSceneStatus;
 if(!in.supplied||!in.source_identity_supplied||!in.source_scale_supplied||!in.caller.supplied||!in.visibility.supplied||!in.visibility.observation.supplied)return S::missing_runtime;
 if(!w.loaded()||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096)return S::invalid_input;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width,au=(size_t)(a.height-1)*a.pitch+a.width;
 if(used>s.bytes||au>a.bytes||world_alias(w,s.pixels,used)||overlap(s.pixels,used,&r,sizeof r)||overlap(s.pixels,used,&d,sizeof d)||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&meta,sizeof meta)||overlap(s.pixels,used,meta.classes,meta.class_bytes)||overlap(s.pixels,used,meta.rules,meta.rule_bytes)||overlap(s.pixels,used,&coast,sizeof coast)||overlap(s.pixels,used,&ot,sizeof ot)||overlap(s.pixels,used,ot.by_detail,ot.count>80?80*sizeof(int32_t):ot.count*sizeof(int32_t))||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,a.pixels,au)||overlap(s.pixels,used,&s,sizeof s))return S::invalid_input;
 const auto&m=w.game().map.info;const auto&v=in.visibility.observation;
 if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||!in.source_map_tiles_address||(uint64_t)in.source_map_tiles_address+(uint64_t)m.tile_count*44>UINT32_MAX)return S::invalid_input;
 if(!in.private_view_fresh_active_cache||in.source_scale_numerator!=0||in.source_scale_denominator!=1||in.render_height!=25||(v.source_zoom!=-5&&v.source_zoom!=5)||in.cache_source_zoom!=v.source_zoom||(v.draw_flags&~1057u)||in.game_preferences)return S::held_view;
 TerrainCallerOrder order{};if(plan_world_terrain_order(w,x,y,v,in.caller,order)!=TerrainCallerStatus::resolved)return S::invalid_input;
 if(order.path!=TerrainCallerPath::land_then_coast&&order.path!=TerrainCallerPath::water)return S::held_view;
 TerrainObservation observed{};auto visible=observe_world_terrain_visibility(w,r,d,x,y,in.visibility,observed);
 if(visible==TerrainVisibilityStatus::missing_runtime||visible==TerrainVisibilityStatus::held_negative_owner_runtime)return S::missing_runtime;
 if(visible!=TerrainVisibilityStatus::resolved)return S::held_observation;
 const size_t index=((size_t)y*m.width+(unsigned)x)/2;const auto*t=w.tile(index);
 // Ordinary plant layers run after ground/water and before coast below.
 // Special landmark heights and unsupported raw material classes remain held.
 if((order.path==TerrainCallerPath::land_then_coast&&(observed.rainfall>2||observed.rockiness>2))||(t->landmarks&0xb03u))return S::held_features;
 // Sea farms use a later Sprite_draw3 path with OS rectangle/scale inputs.
 // Keep that dependency held; the closed land farm texture path is below.
 if(order.path==TerrainCallerPath::water&&(v.draw_flags&1u)&&(observed.displayed_items&0x8000u))return S::held_features;
 TerrainRoadPlan roads{};
 if(v.draw_flags&0x20u){
  // The same source bit also enables crop-yield and later improvement/landmark
  // sprites. Retain those explicit holds rather than silently omitting them.
  constexpr uint32_t later=0x800u|0x40000u|0x80000000u|0x8000u|0x10u|0x40u|0x400000u|0x800000u|0x1000000u;
  TerrainRoadView rv{v.viewer,observed.displayed_items,true};const auto status=world_terrain_roads(w,x,y,rv,roads);
  if(status!=TerrainRoadStatus::resolved)return status==TerrainRoadStatus::held_base_repair?S::held_features:S::failed_stage;
  if(roads.source_late_layers_visible&&((observed.displayed_items&later)||(t->landmarks&0xe000u)))return S::held_features;
 }
 const unsigned zoom=v.source_zoom==-5?1:2;
 auto*memory=(uint8_t*)malloc(used);if(!memory)return S::out_of_memory;memcpy(memory,s.pixels,used);TerrainTextureSurface stage=s;stage.pixels=memory;stage.bytes=used;
 bool ok=false;
 if(order.path==TerrainCallerPath::water){TerrainWaterPatchContext c{in.caller_x,in.caller_y,zoom,observed.visible?-1:65,order.pass_minimum_light,order.pass_maximum_light,0,true,true};ok=render_world_water_patch(w,x,y,c,ot,a,stage);TerrainMaskView view{observed.displayed_items,true};TerrainFeatureInput fi{};fi.mesh.caller_x=in.caller_x;fi.mesh.caller_y=in.caller_y;fi.mesh.viewer_zoom=zoom;fi.altitude=t->climate>>5;fi.fungus=(observed.displayed_items&0x20u)!=0;fi.concealed=observed.concealed;fi.visible=observed.visible;fi.source_context_supplied=true;ok=ok&&world_terrain_masks(w,x,y,view,fi.masks)&&render_terrain_feature_layers(fi,a,stage);}
 else {
  CvrTerrainCorners corners{};TerrainPointLights lights{};TerrainWorldMasks masks{};TerrainMaskView view{observed.displayed_items,true};
  if(!cvr_terrain_corners(w,x,y,zoom,corners)){free(memory);return S::held_height;}
  if(observed.visible){if(!world_terrain_lights(w,x,y,lights)){free(memory);return S::held_height;}}
  else for(auto&light:lights.point)light=2; // Original686354; no bright-cache query.
  if(!world_terrain_masks(w,x,y,view,masks)){free(memory);return S::failed_stage;}
  // The source concealment prelude replaces centre rainfall with zero before
  // neighbour-mask dispatch. Coast topology still uses the real World.
  if(observed.rainfall==0)masks.roughness=0;
  else if(observed.rainfall!=((t->climate>>3)&3)){free(memory);return S::held_observation;}
  TerrainMeshInput mesh{};mesh.caller_x=in.caller_x;mesh.caller_y=in.caller_y;mesh.viewer_zoom=zoom;mesh.height[0]=((t->climate>>5)-3)*((v.source_zoom+16)*25/16);for(unsigned k=0;k<4;++k)mesh.height[k+1]=corners.height[k];
  TerrainDispatchContext material{in.source_map_tiles_address+(uint32_t)index*44,observed.rainfall,observed.rockiness,masks.roughness,(t->landmarks&0x40u)!=0,(observed.displayed_items&0x8000u)!=0,(v.draw_flags&1u)!=0,observed.visible,true};
  TerrainGroundPlateInput ground{mesh,lights,material};TerrainOceanDetails od{};TerrainOceanLights ol{};
  TerrainFeatureInput fi{mesh,masks,(unsigned)(t->climate>>5),(t->landmarks&0x80000004u)==4,(observed.displayed_items&0x20u)!=0,observed.concealed,observed.visible,true};
  fi.forest=(v.draw_flags&1u)&&(observed.displayed_items&0x200000u);fi.lights=lights;
  ok=render_terrain_ground_plate(ground,a,stage)&&render_terrain_feature_layers(fi,a,stage)&&world_ocean_details(w,x,y,od)&&light_ocean_details(od,ot,ol);
  if(ok){TerrainCoastOverlayInput ci{};ci.mesh=mesh;ci.light=ol;for(unsigned k=0;k<4;++k)ci.shore_mask[k]=masks.coast[k];ci.map_x=x;ci.map_y=y;ci.fog_colour=observed.visible?-1:65;ci.source_coast_context_supplied=true;ok=render_terrain_coast_overlay(ci,meta,coast,a,stage);}
  if(ok&&(v.draw_flags&0x400u)){TerrainRiverInput ri{mesh,lights,masks.river,observed.visible?-1:65,(t->items&0x80u)!=0,(t->items&0x200u)!=0,true};ok=render_terrain_rivers(ri,a,stage);}
 }
 if(ok&&(v.draw_flags&0x20u)){TerrainRoadLayerInput ri{};ri.plan=roads;ri.mesh.caller_x=in.caller_x;ri.mesh.caller_y=in.caller_y;ri.mesh.viewer_zoom=zoom;if(order.path==TerrainCallerPath::land_then_coast){CvrTerrainCorners corners{};TerrainPointLights lights{};ok=cvr_terrain_corners(w,x,y,zoom,corners);if(ok&&observed.visible)ok=world_terrain_lights(w,x,y,lights);else if(ok)for(auto&l:lights.point)l=2;ri.mesh.height[0]=((t->climate>>5)-3)*((v.source_zoom+16)*25/16);for(unsigned k=0;k<4;++k)ri.mesh.height[k+1]=corners.height[k];ri.lights=lights;}ok=ok&&render_terrain_roads(ri,a,stage);}
 if(ok)memcpy(s.pixels,memory,used);
 free(memory);return ok?S::rendered:S::failed_stage;
}
}
