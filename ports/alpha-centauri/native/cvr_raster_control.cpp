/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_raster.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
int main(){
 uint8_t pixels[64],shades[2048]={},palette[6144]={};uint16_t depths[64];
 memset(pixels,71,sizeof(pixels));for(unsigned i=0;i<64;++i)depths[i]=12345;
 alpha::CvrRasterPoint points[2]={{256,256,0,0,2},{256,256,0,0,3}};
 alpha::CvrRasterClip clip={0,0,7,7};alpha::CvrRasterSurface target={pixels,depths,64,8,8,8};
 unsigned cases=0;
 for(unsigned bad=0;bad<24;++bad){
  auto s=target;auto c=clip;size_t n=2,ns=2048,np=6144;const auto*p=points;const auto*sh=shades;const auto*pal=palette;
  points[1].normal=0;shades[0]=0;
  switch(bad){
   case 0:p=nullptr;break;case 1:n=2000001;break;case 2:sh=nullptr;break;
   case 3:ns=2047;break;case 4:pal=nullptr;break;case 5:np=6143;break;
   case 6:s.pixels=nullptr;break;case 7:s.depths=nullptr;break;case 8:s.width=1;break;
   case 9:s.height=1;break;case 10:s.width=16385;break;case 11:s.height=16385;break;
   case 12:s.stride=7;break;case 13:s.stride=16385;break;case 14:s.capacity=63;break;
   case 15:c.left=-1;break;case 16:c.top=-1;break;case 17:c.right=0;break;
   case 18:c.bottom=0;break;case 19:c.right=8;break;case 20:c.bottom=8;break;
   case 21:points[1].normal=2048;break;case 22:shades[0]=24;break;case 23:c.right=INT_MAX;break;
  }
  if(alpha::raster_cvr_points(p,n,sh,ns,pal,np,c,true,s))return 20;
  for(unsigned i=0;i<64;++i)if(pixels[i]!=71||depths[i]!=12345)return 20;
  ++cases;
 }
 points[1].normal=0;shades[0]=0;
 if(!alpha::raster_cvr_points(nullptr,0,shades,2048,palette,6144,clip,true,target))return 20;
 ++cases;
 points[0].x=INT_MIN;points[0].y=INT_MAX;
 if(!alpha::raster_cvr_points(points,1,shades,2048,palette,6144,clip,true,target))return 20;
 for(unsigned i=0;i<64;++i)if(pixels[i]!=71||depths[i]!=12345)return 20;
 ++cases;
 printf("CVR RASTER CONTROL cases=%u RC 0\n",cases);return 0;
}
