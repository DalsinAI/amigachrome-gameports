/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_coast.h"
#include <stdio.h>
#include <string.h>
static bool failed=false;
static int32_t read32(){unsigned char b[4];if(fread(b,1,4,stdin)!=4){failed=true;return 0;}return (int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);}
static bool rejects(){
 unsigned char t[3136],m[3136],p[4096],b[4096],lut[]={0,1,37,113};memset(t,42,sizeof t);memset(m,1,sizeof m);memset(p,199,sizeof p);memcpy(b,p,sizeof p);
 alpha::TerrainCoastInput i{{{8,8,1,1,0},{45,8,54,1,0},{45,45,54,54,0},{8,45,1,54,0}},t,sizeof t,m,sizeof m,lut,sizeof lut,56,56,-1};alpha::TerrainTextureSurface s{p,sizeof p,64,64,64};
 auto reject=[&](){return !alpha::draw_terrain_coast(i,s)&&memcmp(p,b,sizeof p)==0;};
 i.texels=p+4095;if(!reject())return false;i.texels=t;
 i.mask=p+4095;if(!reject())return false;i.mask=m;
 i.lookup=p+4095;if(!reject())return false;i.lookup=lut;
 s.pixels=(unsigned char*)&i+1;if(alpha::draw_terrain_coast(i,s))return false;
 s.pixels=(unsigned char*)&s+sizeof(s)-1;if(alpha::draw_terrain_coast(i,s))return false;s.pixels=p;
 i.texture_bytes=3135;if(!reject())return false;i.texture_bytes=3136;
 i.mask_bytes=3135;if(!reject())return false;i.mask_bytes=3136;
 i.lookup_bytes=1;if(!reject())return false;i.lookup_bytes=4;
 i.fog_colour=256;if(!reject())return false;i.fog_colour=-1;
 i.vertex[0].light=128;if(!reject())return false;i.vertex[0].light=0;
 i.vertex[0].u=255;if(!reject())return false;i.vertex[0].u=1;
 s.bytes=4095;if(!reject())return false;s.bytes=4096;
 i.vertex[0].x=4097;if(!reject())return false;i.vertex[0].x=8;
 return true;
}
int main(){if(!rejects())return 20;unsigned char tex[3136],mask[3136],pixels[4096],lut[]={0,1,37,113};for(unsigned i=0;i<3136;++i){tex[i]=(i*17+23)%256;mask[i]=(i%56/3+i/56/5)%4;}for(int n=read32();n>0;--n){alpha::TerrainCoastInput in{};in.texels=tex;in.texture_bytes=sizeof tex;in.mask=mask;in.mask_bytes=sizeof mask;in.lookup=lut;in.lookup_bytes=sizeof lut;in.width=in.height=56;in.fog_colour=read32();for(auto&v:in.vertex){v.x=read32();v.y=read32();v.u=read32();v.v=read32();v.light=read32();}if(failed)return 20;memset(pixels,199,sizeof pixels);alpha::TerrainTextureSurface s{pixels,sizeof pixels,64,64,64};if(!alpha::draw_terrain_coast(in,s))return 20;fwrite(pixels,1,sizeof pixels,stdout);}return failed?20:0;}
