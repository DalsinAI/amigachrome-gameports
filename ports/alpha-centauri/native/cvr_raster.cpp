/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_raster.h"
namespace alpha {
namespace {
/* Explicit floor division: C++ signed / truncates negative fractions. */
int32_t pixel_coordinate(int32_t fixed) {
 int32_t whole=fixed/256;
 return whole-((fixed<0 && fixed%256)!=0);
}
}
bool raster_cvr_points(const CvrRasterPoint* points,size_t count,
 const uint8_t* shades,size_t shade_count,const uint8_t* palette,size_t palette_count,
 const CvrRasterClip& clip,bool quad,const CvrRasterSurface& out) {
 if((count && !points) || count>2000000 || !shades || shade_count!=2048 ||
    !palette || palette_count!=6144 || !out.pixels || !out.depths ||
    out.width<2 || out.height<2 || out.width>16384 || out.height>16384 ||
    out.stride<out.width || out.stride>16384 ||
    out.capacity<size_t(out.stride)*out.height || clip.left<0 || clip.top<0 ||
    clip.right<=clip.left || clip.bottom<=clip.top ||
    uint32_t(clip.right)>=out.width || uint32_t(clip.bottom)>=out.height)return false;
 for(size_t i=0;i<count;++i)
  if(points[i].normal>=2048 || shades[points[i].normal]>=24)return false;
 for(size_t i=0;i<count;++i) {
  const auto& p=points[i];int32_t x=pixel_coordinate(p.x),y=pixel_coordinate(p.y);
  if(quad) {
   if(x<clip.left-1 || y<clip.top-1 || x>clip.right || y>clip.bottom)continue;
   if(x<clip.left)x=clip.left;
   if(y<clip.top)y=clip.top;
   if(x==clip.right)--x;
   if(y==clip.bottom)--y;
  }else if(x<clip.left || y<clip.top || x>=clip.right || y>=clip.bottom)continue;
  const uint16_t depth=uint16_t(p.z>>16);
  const uint8_t color=palette[size_t(shades[p.normal])*256+p.color];
  const unsigned span=quad?2:1;
  for(unsigned dy=0;dy<span;++dy)for(unsigned dx=0;dx<span;++dx) {
   const size_t at=size_t(y+dy)*out.stride+size_t(x+dx);
   if(depth<out.depths[at]){out.depths[at]=depth;out.pixels[at]=color;}
  }
 }
 return true;
}
}
