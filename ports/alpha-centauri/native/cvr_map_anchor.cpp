/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_map_anchor.h"
#include <stddef.h>
#include <limits.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);}
bool fits(int64_t n){return n>=INT32_MIN&&n<=INT32_MAX;}
}
bool cvr_map_metrics(const CvrMapScale&scale,CvrMapMetrics&out){
 if(overlap(&scale,sizeof(scale),&out,sizeof(out))||scale.zoom< -8||scale.zoom>8||scale.numerator<0||scale.numerator>1024||scale.denominator<=0||scale.denominator>1024)return false;
 const int64_t size=scale.numerator?(int64_t)scale.numerator*50/scale.denominator:(int64_t)(scale.zoom+16)*50/16;
 const int64_t unit=(size+1)/4>0?(size+1)/4:1;
 if(!fits(size)||!fits(unit*8))return false;
 out={(int32_t)size,(int32_t)(unit*8),(int32_t)(unit*4),(int32_t)(unit*4),(int32_t)(unit*2)};return true;
}
bool cvr_map_tile_origin(const CvrMapPlacement&c,int32_t x,int32_t y,const int32_t*corners,CvrMapPoint&out){
 if(!corners||overlap(&c,sizeof(c),&out,sizeof(out))||overlap(corners,4*sizeof(*corners),&out,sizeof(out))||c.width<2||c.width>4096||(c.width&1)||c.height<1||c.height>4096||x<0||x>=c.width||y<0||y>=c.height||((x+y)&1)||c.tile_left< -c.width||c.tile_left>c.width||c.tile_top< -c.height||c.tile_top>c.height||c.half_x<1||c.half_x>16384||c.half_y<1||c.half_y>16384||c.odd_x<0||c.odd_x>8192||c.even_x<0||c.even_x>8192||c.odd_y<0||c.odd_y>8192||c.even_y<0||c.even_y>8192)return false;
 int64_t total=0;for(unsigned i=0;i<4;++i){if(corners[i]< -127||corners[i]>127)return false;total+=corners[i];}
 const int64_t altitude=total>=0?total/4:-((-total+3)/4);int64_t representation=x;
 if(!c.flat){if(representation<c.tile_left)representation+=c.width;if(representation>=(int64_t)c.tile_left+c.odd_x+c.even_x)representation-=c.width;}
 int64_t dx=(int64_t)x-c.tile_left;
 if(y>=c.tile_top&&y<(int64_t)c.tile_top+c.odd_y+c.even_y&&representation>=c.tile_left&&representation<(int64_t)c.tile_left+c.odd_x+c.even_x){if(!c.flat){if(dx<0)dx+=c.width;else if(dx>=c.width)dx-=c.width;}}
 else{if(dx>c.width/2)dx-=c.width;if(dx< -c.width/2)dx+=c.width;}
 const int64_t px=(int64_t)c.pixel_left+(dx-1)*c.half_x,py=(int64_t)c.pixel_top+((int64_t)y-c.tile_top-1)*c.half_y-altitude;
 if(!fits(px)||!fits(py))return false;
 out={(int32_t)px,(int32_t)py};return true;
}
bool cvr_viewer_flat_anchor(unsigned zoom,int32_t x,int32_t y,bool infantry,CvrViewerUnitAnchor&out){
 if(zoom!=1&&zoom!=2)return false;
 const CvrMapScale scale{zoom==1?-5:5,0,16};CvrMapMetrics m{};if(!cvr_map_metrics(scale,m)||m.half_x!=(int32_t)zoom*32||m.half_y!=(int32_t)zoom*16)return false;
 const int64_t cx=(int64_t)x-m.half_x,cy=(int64_t)y-2*m.half_y,sx=cx+m.scaled50,sy=cy+m.scaled50;
 if(!fits(cx)||!fits(cy)||!fits(sx)||!fits(sy))return false;
 const float magnification=(float)((double)(scale.zoom+16)*.07f*.0625);
 out={{(int32_t)cx,(int32_t)cy},{(int32_t)sx,(int32_t)sy},scale.zoom,(float)((double)magnification*(infantry?1.25:1.0))};return true;
}
}
