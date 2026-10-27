// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_scene.h"
#include <cstdlib>
#include <cstring>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.game().bunglist.data,w.game().bunglist.size)||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
TerrainSceneStatus render_world_scene_with_sea_platform(const OriginalWorld&w,const Rules&r,const UnitDefinitions&d,int x,int y,const TerrainSceneRuntime&in,const TerrainCoastMetadata&meta,const TerrainCoastResources&coast,const TerrainOceanLightTable&ot,const PreparedTerrainAtlas&a,const TerrainSeaSceneResource&sea,const TerrainTextureSurface&s){using S=TerrainSceneStatus;
 if(!w.loaded()||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096)return S::invalid_input;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width,au=(size_t)(a.height-1)*a.pitch+a.width;
 // Check the ORIGINAL destination before staging: the parent sees a private
 // buffer and otherwise cannot detect aliases with caller-owned descriptors.
 if(used>s.bytes||au>a.bytes||world_alias(w,s.pixels,used)||overlap(s.pixels,used,&r,sizeof r)||overlap(s.pixels,used,&d,sizeof d)||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&meta,sizeof meta)||overlap(s.pixels,used,meta.classes,meta.class_bytes)||overlap(s.pixels,used,meta.rules,meta.rule_bytes)||overlap(s.pixels,used,&coast,sizeof coast)||overlap(s.pixels,used,&ot,sizeof ot)||overlap(s.pixels,used,ot.by_detail,ot.count>80?320:ot.count*4)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,a.pixels,au)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,&sea,sizeof sea)||overlap(s.pixels,used,sea.pixels.pixels,sea.pixels.bytes))return S::invalid_input;
 const auto&m=w.game().map.info;const auto&v=in.visibility.observation;
 if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return S::invalid_input;
 if(!in.supplied||!in.source_identity_supplied||!in.source_scale_supplied||!in.caller.supplied||!in.visibility.supplied||!v.supplied)return S::missing_runtime;
 if(!in.source_map_tiles_address||(uint64_t)in.source_map_tiles_address+(uint64_t)m.tile_count*44>UINT32_MAX)return S::invalid_input;
 if(!in.private_view_fresh_active_cache||in.source_scale_numerator!=0||in.source_scale_denominator!=1||in.render_height!=25||(v.source_zoom!=-5&&v.source_zoom!=5)||in.cache_source_zoom!=v.source_zoom||(v.draw_flags&~0x04000421u)||(in.game_preferences&~0x300u))return S::held_view;
 TerrainCallerOrder order{};if(plan_world_terrain_order(w,x,y,v,in.caller,order)!=TerrainCallerStatus::resolved)return S::invalid_input;
 if(order.path!=TerrainCallerPath::land_then_coast&&order.path!=TerrainCallerPath::water)return S::held_view;
 const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);
 if((t->climate>>5)>=3||!(v.draw_flags&1u))return render_world_ordinary_scene(w,r,d,x,y,in,meta,coast,ot,a,s);
 TerrainObservation observed{};auto status=observe_world_terrain_visibility(w,r,d,x,y,in.visibility,observed);
 if(status==TerrainVisibilityStatus::missing_runtime||status==TerrainVisibilityStatus::held_negative_owner_runtime)return S::missing_runtime;
 if(status!=TerrainVisibilityStatus::resolved)return S::held_observation;
 const bool late=v.viewer<0||(w.faction(v.viewer)->player_flags&0x200u)||(t->visibility&(1u<<v.viewer));
 const bool active=late&&(observed.displayed_items&0x8000u);
 if(active&&(v.draw_flags&0x20u))return S::held_features;
 if(active&&(!sea.coherent_source_cache_supplied||!sea.prepared_owner_pixels_supplied))return S::missing_runtime;
 // bit1 has no other water terrain/observation/caller effect in this normal
 // source profile. It is not removed for land or any unsupported draw mode.
 auto base=in;base.visibility.observation.draw_flags&=~1u;
 const size_t span=(size_t)s.pitch*s.height;auto*mem=static_cast<uint8_t*>(std::malloc(span));if(!mem)return S::out_of_memory;std::memset(mem,0,span);std::memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=span;
 auto result=render_world_ordinary_scene(w,r,d,x,y,base,meta,coast,ot,a,stage);
 if(result==S::rendered&&active){
  TerrainSeaPlatformContext c{in.caller_x,in.caller_y,v.source_zoom,sea.minimum_width,sea.minimum_height,v.draw_flags,observed.displayed_items,(uint32_t)(t->climate>>5),late,true,true};
  SpritePixelSurface dest{mem,span,(int32_t)s.width,(int32_t)s.height,(int32_t)s.pitch};
  const auto drawn=draw_terrain_sea_platform(c,sea.clip,sea.pixels,dest);
  if(drawn!=TerrainSeaPlatformStatus::rendered&&drawn!=TerrainSeaPlatformStatus::clipped&&drawn!=TerrainSeaPlatformStatus::minimum_size)result=S::failed_stage;
 }
 if(result==S::rendered)std::memcpy(s.pixels,mem,used);
 std::free(mem);return result;
}
}
