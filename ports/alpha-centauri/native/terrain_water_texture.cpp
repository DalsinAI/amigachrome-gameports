/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_water_texture.h"
#include <limits.h>
namespace alpha {namespace {
bool overlap(const void*a,size_t an,const void*b,size_t bn){
 const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
 if(an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y)return true;
 return an&&bn&&x<y+bn&&y<x+an;
}
unsigned bits(uint64_t x){unsigned n=0;while(x){++n;x>>=1;}return n;}
int64_t gradient(int64_t delta,unsigned width,unsigned scale){
 const uint64_t d=(uint64_t)width<<16;
 const unsigned top=23+bits(d-1);
 const uint64_t mant=(UINT64_C(1)<<top)/d;
 uint64_t p=(uint64_t)(delta<0?-delta:delta)*mant;
 const unsigned b=bits(p),cut=b>24?b-24:0;
 p=(p>>cut)<<cut;
 const int64_t result=(int64_t)(p>>(top-scale));
 return delta<0?-result:result;
}
int64_t floor_div(int64_t n,int64_t d){return n>=0?n/d:-((-n+d-1)/d);}
struct Edge {int64_t x,u,v,l;};
bool edge_at(const TerrainTextureVertex*v,unsigned top,int direction,int y,Edge&e){
 unsigned a=top;
 for(unsigned i=0;i<4;++i){
  const unsigned b=(a+4+direction)%4;
  const int dy=v[b].y-v[a].y;
  if(dy>0&&y>=v[a].y&&y<v[b].y){
   const int t=y-v[a].y;
   const int64_t dx=(int64_t)(v[b].x-v[a].x)*t;
   e.x=v[a].x-floor_div(-dx,dy);
   const int64_t u=(int64_t)v[a].u*65536+32768;
   const int64_t w=(int64_t)v[a].v*65536+32768;
   e.u=u+(((int64_t)v[b].u*65536-u)/dy)*t;
   e.v=w+(((int64_t)v[b].v*65536-w)/dy)*t;
   e.l=(int64_t)v[a].light*65536+((int64_t)(v[b].light-v[a].light)*65536/dy)*t;
   return true;
  }
  if(dy<0)return false;
  a=b;
 }
 return false;
}
bool pass(const TerrainWaterTextureInput&in,const TerrainTextureSurface&s,bool write,int fog,bool thresholded,int threshold){
 unsigned top=0;int bottom=in.vertex[0].y;
 for(unsigned i=1;i<4;++i){if(in.vertex[i].y<in.vertex[top].y)top=i;if(in.vertex[i].y>bottom)bottom=in.vertex[i].y;}
 const int first=in.vertex[top].y<0?0:in.vertex[top].y,end=bottom>(int)s.height?(int)s.height:bottom;
 for(int y=first;y<end;++y){
  Edge l{},r{};if(!edge_at(in.vertex,top,-1,y,l)||!edge_at(in.vertex,top,1,y,r))return false;if(r.x<=l.x)continue;
  const unsigned width=(unsigned)(r.x-l.x);const int64_t du=gradient(r.u-l.u,width,16),dv=gradient(r.v-l.v,width,16),dl=gradient(r.l-l.l,width,8);
  const int start=l.x<0?0:(int)l.x,finish=r.x>(int)s.width?(int)s.width:(int)r.x;
  for(int x=start;x<finish;++x){
   if(fog>=0&&(y&1)==0){if(write)s.pixels[(size_t)y*s.pitch+x]=(uint8_t)fog;continue;}
   const int64_t t=x-l.x,sample=fog>=0&&!thresholded&&x>start?t-1:t;
   const int64_t u=floor_div(l.u+du*sample,65536),v=floor_div(l.v+dv*sample,65536);
   if(u<0||v<0||u>=in.width||v>=in.height)return false;
   const uint16_t shade=(uint16_t)(floor_div(l.l,256)+dl*t);const uint8_t high=(uint8_t)(shade>>8);
   if(write&&(!thresholded||(int8_t)high<threshold))s.pixels[(size_t)y*s.pitch+x]=(uint8_t)(in.texels[(size_t)v*in.width+u]+high);
  }
 }
 return true;
}

}
static bool draw_water(const TerrainWaterTextureInput&input,const TerrainTextureSurface&surface,int fog,bool thresholded,int threshold,int minimum,int maximum){
 if(!input.texels||!input.width||!input.height||input.width>256||input.height>256||fog< -1||fog>255||threshold< -127||threshold>127)return false;
 if(!thresholded&&(minimum< -256||maximum>256||minimum>maximum))return false;
 const size_t texused=(size_t)input.width*input.height;if(input.texture_bytes<texused)return false;
 if(!surface.pixels||!surface.width||!surface.height||surface.width>4096||surface.height>4096||surface.pitch<surface.width||surface.pitch>65536)return false;
 const size_t used=(size_t)(surface.height-1)*surface.pitch+surface.width;if(used>surface.bytes||overlap(surface.pixels,used,&input,sizeof input)||overlap(surface.pixels,used,&surface,sizeof surface)||overlap(surface.pixels,used,input.texels,texused))return false;
 TerrainWaterTextureInput in=input;bool all_at_or_above=true;for(auto&v:in.vertex){if(v.x< -4096||v.x>4096||v.y< -4096||v.y>4096||v.u<0||v.u>255||v.v<0||v.v>255||v.light< -127||v.light>127)return false;if(v.light<threshold)all_at_or_above=false;if(!thresholded){if(v.light<minimum)v.light=minimum;if(v.light>maximum)v.light=maximum;if(v.light< -127||v.light>127)return false;}}
 if(thresholded&&all_at_or_above)return true;
 return pass(in,surface,false,fog,thresholded,threshold)&&pass(in,surface,true,fog,thresholded,threshold);
}
bool draw_terrain_water_primary(const TerrainWaterTextureInput&i,const TerrainTextureSurface&s,int minimum,int maximum,int fog){return draw_water(i,s,fog,false,0,minimum,maximum);}
bool draw_terrain_water_threshold(const TerrainWaterTextureInput&i,const TerrainTextureSurface&s,int threshold,int fog){return draw_water(i,s,fog,true,threshold,0,0);}
}
