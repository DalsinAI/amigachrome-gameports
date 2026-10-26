/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_water_patch.h"
#include "terrain_water_texture.h"
#include <stdlib.h>
#include <string.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
bool world_alias(const OriginalWorld&w,const void*p,size_t n){const auto&m=w.game().map.info;return overlap(p,n,&w,sizeof w)||overlap(p,n,w.original_bytes(),w.original_size())||overlap(p,n,w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlap(p,n,w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlap(p,n,w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlap(p,n,w.prototype(0),512*sizeof(OriginalPrototype))||overlap(p,n,w.faction(0),8*sizeof(OriginalFactionState))||overlap(p,n,w.faction_definition(0),8*sizeof(OriginalFactionDefinition));}
}
bool build_world_water_plan(const OriginalWorld&w,int x,int y,const TerrainWaterPatchContext&in,const TerrainOceanLightTable&t,TerrainWaterPlan&out){
 const auto&m=w.game().map.info;if(!w.loaded()||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1)||!in.source_context_supplied||!in.source_bounds_supplied||(in.source_draw_flags&0x82000000u)||(in.viewer_zoom!=1&&in.viewer_zoom!=2)||(in.fog_colour!= -1&&in.fog_colour!=65)||in.minimum_light< -256||in.maximum_light>256||in.minimum_light>in.maximum_light||in.minimum_light>127||in.maximum_light< -127)return false;
 if(world_alias(w,&out,sizeof out)||overlap(&out,sizeof out,&in,sizeof in)||overlap(&out,sizeof out,&t,sizeof t)||overlap(&out,sizeof out,t.by_detail,80*sizeof(int32_t)))return false;
 const auto*tile=w.tile(((size_t)y*m.width+(unsigned)x)/2);if(!tile||(tile->climate>>5)>=3||!t.supplied||!t.by_detail||t.count<80)return false;for(unsigned i=0;i<80;++i)if(t.by_detail[i]<0||t.by_detail[i]>37)return false;
 TerrainOceanDetails d{};TerrainOceanLights lights{};if(!world_ocean_details(w,x,y,d)||!light_ocean_details(d,t,lights))return false;
 const int half=in.viewer_zoom==1?16:32,quarter=half/2;const int64_t cx=(int64_t)in.caller_x+2*half,cy=(int64_t)in.caller_y+half;const int dx[4]={-2,0,2,0},dy[4]={0,-2,0,2};TerrainWaterPlan result{};result.fog_colour=in.fog_colour;
 for(unsigned s=0;s<4;++s)for(unsigned v=0;v<4;++v){int a=0,b=0;if(v==2){a=dx[s]*half;b=dy[s]*quarter;}else if(v==1||v==3){unsigned neighbour=(s+(v==1?3:1))&3;a=(dx[s]+dx[neighbour])*half/2;b=(dy[s]+dy[neighbour])*quarter/2;}int64_t px=cx+a,py=cy+b;if(px< -4096||px>4096||py< -4096||py>4096)return false;int u=27,z=27;if(v){if(s==0){u=v==1?27:0;z=v==3?0:55;}else if(s==1){u=v==1?27:55;z=v==3?55:0;}else if(s==2){u=v==3?27:55;z=v==1?27:55;}else{u=v==3?27:0;z=v==1?27:0;}}result.vertex[s][v]={(int32_t)px,(int32_t)py,u,z,lights.point[s][v]};if(lights.point[s][v]>=16)result.deep[s]=true;}
 out=result;return true;
}
bool render_world_water_patch(const OriginalWorld&w,int x,int y,const TerrainWaterPatchContext&in,const TerrainOceanLightTable&t,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){
 if(!a.pixels||a.width<336||a.height<192||a.width>4096||a.height>4096||a.pitch<a.width||a.pitch>4096||!s.pixels||!s.width||!s.height||s.width>256||s.height>256||s.pitch<s.width||s.pitch>256)return false;
 const size_t au=(size_t)(a.height-1)*a.pitch+a.width,used=(size_t)(s.height-1)*s.pitch+s.width;
 if(au>a.bytes||used>s.bytes||world_alias(w,s.pixels,used)||overlap(s.pixels,used,&in,sizeof in)||overlap(s.pixels,used,&t,sizeof t)||overlap(s.pixels,used,t.by_detail,80*sizeof(int32_t))||overlap(s.pixels,used,&a,sizeof a)||overlap(s.pixels,used,&s,sizeof s)||overlap(s.pixels,used,a.pixels,au))return false;
 TerrainWaterPlan plan{};if(!build_world_water_plan(w,x,y,in,t,plan))return false;
 uint8_t*memory=(uint8_t*)malloc(used+6272);if(!memory)return false;memcpy(memory,s.pixels,used);uint8_t*primary=memory+used,*deep=primary+3136;for(unsigned row=0;row<56;++row){memcpy(primary+row*56,a.pixels+(size_t)(79+row)*a.pitch+280,56);memcpy(deep+row*56,a.pixels+(size_t)(136+row)*a.pitch+280,56);}TerrainTextureSurface stage=s;stage.pixels=memory;stage.bytes=used;bool ok=true;
 for(unsigned k=0;k<4&&ok;++k){TerrainWaterTextureInput draw{};draw.width=draw.height=56;draw.texture_bytes=3136;for(unsigned v=0;v<4;++v)draw.vertex[v]=plan.vertex[k][v];if(plan.deep[k]){draw.texels=deep;for(auto&v:draw.vertex)v.light-=16;ok=draw_terrain_water_primary(draw,stage,in.minimum_light,in.maximum_light,in.fog_colour);}if(ok){draw.texels=primary;for(unsigned v=0;v<4;++v)draw.vertex[v]=plan.vertex[k][v];ok=draw_terrain_water_threshold(draw,stage,16,in.fog_colour);}}
 if(ok)memcpy(s.pixels,memory,used);
 free(memory);return ok;
}
}
