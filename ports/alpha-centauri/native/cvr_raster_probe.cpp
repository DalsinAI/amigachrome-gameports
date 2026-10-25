/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cvr_raster.h"
#include <stdio.h>
#include <string.h>
static uint32_t u32(const unsigned char*b){return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
static bool read(FILE*f,void*b,size_t n){return fread(b,1,n,f)==n;}
int main(int argc,char**argv){
 if(argc!=2 && argc!=3)return 20;
 FILE*f=fopen(argv[1],"rb");if(!f)return 20;
 unsigned char header[8],shades[2048],palette[6144],pixels[320],wire[640];uint16_t depths[320];
 if(!read(f,header,8)||memcmp(header,"CVRP",4)||u32(header+4)>4096||!read(f,shades,sizeof(shades))||!read(f,palette,sizeof(palette))){fclose(f);return 20;}
 const uint32_t batches=u32(header+4);FILE* output=argc==3?fopen(argv[2],"wb"):stdout;
 if(!output){fclose(f);return 20;}
 for(uint32_t batch=0;batch<batches;++batch){
  unsigned char metadata[24],point[15];alpha::CvrRasterPoint points[64];
  if(!read(f,metadata,24)||!read(f,pixels,sizeof(pixels))||!read(f,wire,sizeof(wire))){fclose(f);return 20;}
  for(unsigned i=0;i<320;++i)depths[i]=uint16_t(wire[2*i]|(uint16_t(wire[2*i+1])<<8));
  uint32_t count=u32(metadata+20);if(count>64||u32(metadata+16)>1){fclose(f);return 20;}
  for(uint32_t i=0;i<count;++i){
   if(!read(f,point,15)){fclose(f);return 20;}
   /* memcpy preserves signed bit patterns without implementation-defined cast. */
   uint32_t x=u32(point),y=u32(point+4);memcpy(&points[i].x,&x,4);memcpy(&points[i].y,&y,4);
   points[i].z=u32(point+8);points[i].normal=uint16_t(point[12]|(uint16_t(point[13])<<8));points[i].color=point[14];
  }
  alpha::CvrRasterClip clip={int32_t(u32(metadata)),int32_t(u32(metadata+4)),int32_t(u32(metadata+8)),int32_t(u32(metadata+12))};
  alpha::CvrRasterSurface target={pixels,depths,320,16,16,20};
  if(!alpha::raster_cvr_points(points,count,shades,sizeof(shades),palette,sizeof(palette),clip,u32(metadata+16)!=0,target)){fclose(f);return 20;}
  for(unsigned i=0;i<320;++i){wire[2*i]=uint8_t(depths[i]);wire[2*i+1]=uint8_t(depths[i]>>8);}
  if(fwrite(pixels,1,sizeof(pixels),output)!=sizeof(pixels)||fwrite(wire,1,sizeof(wire),output)!=sizeof(wire)){fclose(f);if(output!=stdout)fclose(output);return 20;}
 }
 bool end=fgetc(f)==EOF&&!ferror(f);fclose(f);if(output!=stdout && fclose(output)!=0)end=false;return end?0:20;
}
