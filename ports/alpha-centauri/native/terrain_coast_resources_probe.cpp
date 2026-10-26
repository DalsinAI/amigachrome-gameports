/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "terrain_coast_resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t*readfile(const char*path,size_t&n){FILE*f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,2)){fclose(f);return nullptr;}long size=ftell(f);rewind(f);if(size<1||size>1024*1024){fclose(f);return nullptr;}auto*p=(uint8_t*)malloc(size);if(!p){fclose(f);return nullptr;}n=size;if(fread(p,1,n,f)!=n){free(p);p=nullptr;}fclose(f);return p;}
static bool failed=false;
static int32_t read32(){uint8_t b[4];if(fread(b,1,4,stdin)!=4){failed=true;return 0;}return (int32_t)((uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24);}
static void put32(int32_t x){uint32_t u=(uint32_t)x;for(unsigned i=0;i<4;++i)putchar((uint8_t)(u>>(i*8)));}
static bool controls(uint8_t*rain,size_t n,const alpha::TerrainCoastMetadata&m,const alpha::TerrainCoastResources&r){
 alpha::TerrainCoastResources out=r;auto bad=m;bad.supplied=false;if(alpha::prepare_terrain_coast_resources(rain,n,640,480,bad,out)||memcmp(&out,&r,sizeof r))return false;bad=m;bad.class_bytes=52;if(alpha::prepare_terrain_coast_resources(rain,n,640,480,bad,out))return false;bad=m;bad.rule_bytes=423;if(alpha::prepare_terrain_coast_resources(rain,n,640,480,bad,out))return false;
 if(alpha::prepare_terrain_coast_resources(rain,n-1,640,480,m,out))return false;
 const void*ptr[]={rain+1,m.classes+1,m.rules+1,&m};for(auto p:ptr)if(alpha::prepare_terrain_coast_resources(rain,n,640,480,m,*reinterpret_cast<alpha::TerrainCoastResources*>(const_cast<void*>(p))))return false;
 unsigned i=333*640+2;uint8_t old=rain[i];rain[i]=255;bool ok=alpha::prepare_terrain_coast_resources(rain,n,640,480,m,out);rain[i]=old;if(ok||memcmp(&out,&r,sizeof r))return false;
 alpha::TerrainMesh mesh{};alpha::TerrainCoastSector sector{};memset(&sector,99,sizeof sector);auto before=sector;if(alpha::prepare_terrain_coast_sector(mesh,4,1,0,0,m,r,sector)||memcmp(&before,&sector,sizeof sector))return false;if(alpha::prepare_terrain_coast_sector(mesh,0,8,0,0,m,r,sector)||memcmp(&before,&sector,sizeof sector))return false;
 const void*q[]={&mesh,&r,&m,m.classes+4,m.rules+4};for(auto p:q)if(alpha::prepare_terrain_coast_sector(mesh,0,1,0,0,m,r,*reinterpret_cast<alpha::TerrainCoastSector*>((uintptr_t)p&~uintptr_t(3))))return false;
 return true;
}
int main(int argc,char**argv){if(argc!=3)return 20;size_t n=0,m=0;auto*rain=readfile(argv[1],n);auto*meta=readfile(argv[2],m);if(!rain||!meta||m!=477){free(rain);free(meta);return 20;}alpha::TerrainCoastMetadata info{meta,53,meta+53,424,true};alpha::TerrainCoastResources resource{};if(!alpha::prepare_terrain_coast_resources(rain,n,640,480,info,resource)){free(rain);free(meta);return 20;}if(!controls(rain,n,info,resource)){free(rain);free(meta);return 20;}fwrite(resource.mask,1,sizeof resource.mask,stdout);fwrite(resource.palette,1,sizeof resource.palette,stdout);
 int count=read32();if(count<0||count>10000)return 20;for(int i=0;i<count;++i){alpha::TerrainMeshInput in{};in.caller_x=read32();in.caller_y=read32();for(auto&v:in.height)v=read32();in.viewer_zoom=(unsigned)read32();int x=read32(),y=read32();unsigned sector=(unsigned)read32();uint8_t mask=(uint8_t)read32();alpha::TerrainMesh mesh{};alpha::TerrainCoastSector out{};if(failed||!alpha::build_terrain_mesh(in,mesh)||!alpha::prepare_terrain_coast_sector(mesh,sector,mask,x,y,info,resource,out)){free(rain);free(meta);return 20;}for(const auto&p:out.xy)for(auto v:p)put32(v);for(const auto&v:out.uv){put32(v.u);put32(v.v);}fwrite(out.lookup,1,sizeof out.lookup,stdout);}
 free(rain);free(meta);return failed?20:0;
}
