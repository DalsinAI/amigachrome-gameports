/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_terrain_context.h"
namespace alpha {namespace {
bool overlaps(const void*a,size_t n,const void*b,size_t m){if(!n||!m)return false;const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return n>UINTPTR_MAX-x||m>UINTPTR_MAX-y||(x<y+m&&y<x+n);}
const OriginalMapTile* tile(const OriginalWorld&w,int x,int y){
 const auto&m=w.game().map.info;
 if(!m.flat){if(x<0)x+=(int)m.width;else if(x>=(int)m.width)x-=(int)m.width;}
 if(x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return nullptr;
 return w.tile(((size_t)y*m.width+(unsigned)x)/2);
}
}
bool cvr_terrain_corners(const OriginalWorld&w,int x,int y,unsigned zoom,CvrTerrainCorners&out){
 const auto&m=w.game().map.info;
 if(!w.loaded()||(zoom!=1&&zoom!=2)||x<0||y<0||x>=(int)m.width||y>=(int)m.height||((x+y)&1))return false;
 const auto*centre=tile(w,x,y);if(!centre||((centre->climate>>5)<3||(centre->climate>>5)>6)||(centre->landmarks&0xb03u))return false;
 // Prevent output writes through the immutable world or any owned allocation.
 if(overlaps(&out,sizeof(out),&w,sizeof(w))||overlaps(&out,sizeof(out),w.original_bytes(),w.original_size())||overlaps(&out,sizeof(out),w.tile(0),(size_t)m.tile_count*sizeof(OriginalMapTile))||overlaps(&out,sizeof(out),w.vehicle(0),(size_t)w.game().vehicle_count*sizeof(OriginalVehicle))||overlaps(&out,sizeof(out),w.base(0),(size_t)w.game().base_count*sizeof(OriginalBase))||overlaps(&out,sizeof(out),w.prototype(0),512*sizeof(OriginalPrototype))||overlaps(&out,sizeof(out),w.faction(0),8*sizeof(OriginalFactionState))||overlaps(&out,sizeof(out),w.faction_definition(0),8*sizeof(OriginalFactionDefinition)))return false;
 const int dx[4][4]={{-2,-1,0,-1},{-1,0,1,0},{0,1,2,1},{-1,0,1,0}};
 const int dy[4][4]={{0,-1,0,1},{-1,-2,-1,0},{0,-1,0,1},{1,0,1,2}};
 const int render_height=((zoom==1?-5:5)+16)*25/16;CvrTerrainCorners result{};
 for(unsigned k=0;k<4;++k){int elevation_sum=0,height_sum=0;bool shore=false;
  for(unsigned j=0;j<4;++j){const auto*t=tile(w,x+dx[k][j],y+dy[k][j]);if(!t||(t->climate>>5)<3){shore=true;break;}if((t->climate>>5)>6||(t->landmarks&0xb03u))return false;const int level=t->climate>>5;elevation_sum+=level;height_sum+=(level-3)*render_height;}
  if(shore){result.relative[k]=4;result.height[k]=0;}else{const int relative=elevation_sum-(centre->climate>>5)*4+4;if(relative<1||relative>7)return false;result.relative[k]=(uint8_t)relative;result.height[k]=height_sum/4;}
 }
 out=result;return true;
}
}
