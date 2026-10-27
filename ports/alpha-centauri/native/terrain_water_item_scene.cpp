// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_water_item_scene.h"
#include "terrain_roads.h"
#include <cstdlib>
#include <cstring>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;auto x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.game().bunglist.data,w.game().bunglist.size)||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
TerrainSceneStatus render_world_water_item_scene(const OriginalWorld&w,const Rules&r,const UnitDefinitions&d,int x,int y,const TerrainSceneRuntime&in,const TerrainCoastMetadata&meta,const TerrainCoastResources&coast,const TerrainOceanLightTable&ot,const PreparedTerrainAtlas&a,const TerrainWaterItemResources&resources,const TerrainTextureSurface&s){using S=TerrainSceneStatus;const auto&sea=resources.sea;
 if(!w.loaded()||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256||!a.pixels||!a.width||!a.height||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096)return S::invalid_input;
 const size_t used=(size_t)(s.height-1)*s.pitch+s.width,au=(size_t)(a.height-1)*a.pitch+a.width;
 // Check the ORIGINAL destination before staging: the parent sees a private
 // buffer and otherwise cannot detect aliases with caller-owned descriptors.
 if(used>s.bytes||au>a.bytes||world_alias(w,s.pixels,used)||overlap(s.pixels,used,&r,sizeof r)||overlap(s.pixels,used,&d,sizeof d)||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&meta,sizeof meta)||overlap(s.pixels,used,meta.classes,meta.class_bytes)||overlap(s.pixels,used,meta.rules,meta.rule_bytes)||overlap(s.pixels,used,&coast,sizeof coast)||overlap(s.pixels,used,&ot,sizeof ot)||overlap(s.pixels,used,ot.by_detail,ot.count>80?320:ot.count*4)||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,a.pixels,au)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,&sea,sizeof sea)||overlap(s.pixels,used,sea.pixels.pixels,sea.pixels.bytes))return S::invalid_input;
 if(overlap(s.pixels,used,&resources,sizeof resources))return S::invalid_input;
 for(const auto&item:resources.items.item)if(overlap(s.pixels,used,item.pixels.pixels,item.pixels.bytes))return S::invalid_input;
 const auto&m=w.game().map.info;const auto&v=in.visibility.observation;
 if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return S::invalid_input;
 if(!in.supplied||!in.source_identity_supplied||!in.source_scale_supplied||!in.caller.supplied||!in.visibility.supplied||!v.supplied)return S::missing_runtime;
 if(!in.source_map_tiles_address||(uint64_t)in.source_map_tiles_address+(uint64_t)m.tile_count*44>UINT32_MAX)return S::invalid_input;
 if(!in.private_view_fresh_active_cache||in.source_scale_numerator!=0||in.source_scale_denominator!=1||in.render_height!=25||(v.source_zoom!=-5&&v.source_zoom!=5)||in.cache_source_zoom!=v.source_zoom||(v.draw_flags&~0x04000421u)||(in.game_preferences&~0x300u))return S::held_view;
 TerrainCallerOrder order{};if(plan_world_terrain_order(w,x,y,v,in.caller,order)!=TerrainCallerStatus::resolved)return S::invalid_input;
 if(order.path!=TerrainCallerPath::land_then_coast&&order.path!=TerrainCallerPath::water)return S::held_view;
 const auto*t=w.tile(((size_t)y*m.width+(unsigned)x)/2);
 if((t->climate>>5)>=3)return render_world_scene_with_sea_platform(w,r,d,x,y,in,meta,coast,ot,a,sea,s);
 TerrainObservation observed{};auto status=observe_world_terrain_visibility(w,r,d,x,y,in.visibility,observed);
 if(status==TerrainVisibilityStatus::missing_runtime||status==TerrainVisibilityStatus::held_negative_owner_runtime)return S::missing_runtime;
 if(status!=TerrainVisibilityStatus::resolved)return S::held_observation;
 const bool late=v.viewer<0||(w.faction(v.viewer)->player_flags&0x200u)||(t->visibility&(1u<<v.viewer));
 SeaLateContext c{v.viewer,(int32_t)v.current_faction,(int32_t)(t->val3&7),x,y,v.source_zoom,(uint32_t)(t->climate>>5),v.draw_flags,observed.displayed_items,t->landmarks,late,in.visibility.expansion_enabled,true,true};SeaLatePlan plan{};
 const auto intent=inspect_sea_late(c,&plan);if(intent==SeaLateStatus::missing_context)return S::missing_runtime;if(intent==SeaLateStatus::held_mode)return S::held_features;if(intent!=SeaLateStatus::inspected)return S::invalid_input;
 bool sea_active=false,items_active=false;
 for(unsigned i=0;i<plan.count;++i){const auto op=plan.events[i].operation;if(op==SeaLateOperation::crop_yield||op==SeaLateOperation::item_highbit)return S::held_features;if(op==SeaLateOperation::sea_platform)sea_active=true;else items_active=true;}
 if(late&&(v.draw_flags&0x20u)&&(observed.displayed_items&(0x800u|0x40000u)))return S::held_features;
 if(v.draw_flags&0x20u){TerrainRoadPlan rp{};TerrainRoadView rv{v.viewer,observed.displayed_items,true};const auto road=world_terrain_roads(w,x,y,rv,rp);if(road!=TerrainRoadStatus::resolved)return road==TerrainRoadStatus::held_base_repair?S::held_features:S::failed_stage;}
 if(items_active&&(!resources.draw.supplied||!resources.draw.coherent_source_cache_supplied))return S::missing_runtime;
 if(items_active&&(resources.draw.caller_x!=in.caller_x||resources.draw.caller_y!=in.caller_y))return S::invalid_input;
 if(sea_active&&(!sea.coherent_source_cache_supplied||!sea.prepared_owner_pixels_supplied))return S::missing_runtime;
 // Source water never draws road pixels. The qualified pure base query above
 // preserves its repair hold before bit20 is split off for late item rendering.
 auto base=in;base.visibility.observation.draw_flags&=~0x20u;
 const size_t span=(size_t)s.pitch*s.height;auto*mem=static_cast<uint8_t*>(std::malloc(span));if(!mem)return S::out_of_memory;std::memset(mem,0,span);std::memcpy(mem,s.pixels,used);auto stage=s;stage.pixels=mem;stage.bytes=span;
 auto result=render_world_scene_with_sea_platform(w,r,d,x,y,base,meta,coast,ot,a,sea,stage);
 if(result==S::rendered&&items_active){auto later=c;later.draw_flags&=~1u;SpritePixelSurface dest{mem,span,(int32_t)s.width,(int32_t)s.height,(int32_t)s.pitch};const auto drawn=render_water_item_sprites(later,resources.draw,resources.items,dest);if(drawn==WaterItemDrawStatus::missing_runtime)result=S::missing_runtime;else if(drawn==WaterItemDrawStatus::held_crop||drawn==WaterItemDrawStatus::held_operation||drawn==WaterItemDrawStatus::held_profile)result=S::held_features;else if(drawn!=WaterItemDrawStatus::rendered)result=S::failed_stage;}
 if(result==S::rendered)std::memcpy(s.pixels,mem,used);
 std::free(mem);return result;
}
}
