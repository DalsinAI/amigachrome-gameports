/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_minimap.h"
#include <string.h>
namespace alpha { namespace {
void put(const UiFrame& f,int x,int y,const uint8_t* rgb){if(x<0 || y<0 || x>=(int)f.width || y>=(int)f.height)return;uint8_t* p=f.rgb+((size_t)y*f.width+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
const OriginalMapTile* sample(const MapTiles& m,const UiRect& r,int px,int py,int& x,int& y){
 y=(int)((int64_t)py*m.height/r.h);x=(int)((int64_t)px*m.width/r.w);
 if((x+y)&1)x=x+1<(int)m.width?x+1:x-1;
 return map_tile_at(m,x,y);
}
}
bool minimap_to_map(const MapTiles& m,const UiRect& r,int px,int py,int& x,int& y){
 if(r.w<=0 || r.h<=0 || px<r.x || py<r.y || px>=r.x+r.w || py>=r.y+r.h || !m.width || !m.height)return false;
 int tx,ty;if(!sample(m,r,px-r.x,py-r.y,tx,ty))return false;x=tx;y=ty;return true;
}
bool draw_minimap(const UiFrame& f,const UiRect& r,const MapTiles& m,const MinimapStyle& s,const MinimapMarker* markers,size_t count,const MapCamera* cam,MinimapStats* stats){
 if(!ui_frame_valid(f) || r.w<=0 || r.h<=0 || r.w>4096 || r.h>4096 || !m.tiles || !m.width || !m.height || m.width>4096 || m.height>4096 || m.count<((size_t)m.width*m.height)/2 || (count && !markers))return false;
 MinimapStats st{};
 for(int py=0;py<r.h;++py)for(int px=0;px<r.w;++px){
  int x,y;const OriginalMapTile* t=sample(m,r,px,py,x,y);if(!t)continue;
  ++st.pixels;
  if(!map_tile_explored(*t,s.viewer)){put(f,r.x+px,r.y+py,s.unexplored);continue;}
  ++st.explored;
  const uint32_t items=map_tile_items(*t,s.viewer);uint8_t c[3];
  const uint8_t* base=map_tile_ocean(*t)?((t->climate&0xe0u)==0x40?s.shelf:s.ocean):((t->val3&0xc0u)>=0x80?s.rocky:s.land);
  if(items&map_bit::fungus)base=s.fungus;
  memcpy(c,base,3);
  if(t->owner>0 && t->owner<8 && (s.faction_mask>>t->owner&1) && !map_tile_ocean(*t)){
   for(int k=0;k<3;++k)c[k]=(uint8_t)((c[k]+3*s.factions[t->owner][k])/4);
   ++st.owned;}
  put(f,r.x+px,r.y+py,c);
 }
 // Units first, bases over them.
 for(unsigned pass=0;pass<2;++pass)for(size_t i=0;i<count;++i){const auto& mk=markers[i];if(mk.base!=(pass==1))continue;
  const OriginalMapTile* t=map_tile_at(m,mk.x,mk.y);if(!t || !map_tile_explored(*t,s.viewer) || mk.faction<0 || mk.faction>7)continue;
  const int px=r.x+(int)((int64_t)mk.x*r.w/m.width),py=r.y+(int)((int64_t)mk.y*r.h/m.height);
  static const uint8_t white[3]={240,240,240};
  const uint8_t* rgb=(s.faction_mask>>mk.faction&1)?s.factions[mk.faction]:white;
  if(mk.base){for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(px+dx>=r.x && px+dx<r.x+r.w && py+dy>=r.y && py+dy<r.y+r.h)put(f,px+dx,py+dy,(dx||dy)?rgb:white);++st.bases;}
  else{put(f,px,py,rgb);++st.units;}
 }
 if(cam && cam->zoom>=1 && cam->zoom<=2 && cam->width && cam->height){
  const int half_w=(int)cam->width/(64*cam->zoom),half_h=(int)cam->height/(32*cam->zoom);
  const int x0=(int)((int64_t)(cam->x-half_w)*r.w/(int)m.width),x1=(int)((int64_t)(cam->x+half_w)*r.w/(int)m.width);
  const int y0=(int)((int64_t)(cam->y-half_h)*r.h/(int)m.height),y1=(int)((int64_t)(cam->y+half_h)*r.h/(int)m.height);
  auto plot=[&](int x,int y){
   if(!m.flat){x%=r.w;if(x<0)x+=r.w;}
   if(x<0 || x>=r.w || y<0 || y>=r.h)return;
   put(f,r.x+x,r.y+y,s.view);++st.view_pixels;};
  for(int x=x0;x<=x1 && x-x0<r.w;++x){plot(x,y0);plot(x,y1);}
  for(int y=y0+1;y<y1;++y){plot(x0,y);plot(x1,y);}
 }
 if(stats)*stats=st;
 return true;
}
}
