// SPDX-License-Identifier: GPL-3.0-or-later
#include "terrain_sea_scene.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static unsigned char sea_pixels[2880];
static alpha::TerrainSeaSceneResource sea{{sea_pixels,sizeof(sea_pixels),72,40,72,9},{0,0,128,128},0,0,true,true};
static bool padded_profile=false;
namespace alpha {
static TerrainSceneStatus scene_adapter(const OriginalWorld&w,const Rules&r,const UnitDefinitions&d,int x,int y,const TerrainSceneRuntime&in,const TerrainCoastMetadata&m,const TerrainCoastResources&c,const TerrainOceanLightTable&o,const PreparedTerrainAtlas&a,const TerrainTextureSurface&s){static bool checked=false;static const unsigned char*regular_pixels=nullptr;
 if(!checked){checked=true;regular_pixels=s.pixels;unsigned char desc[sizeof(sea)],pixels[sizeof(sea_pixels)];std::memcpy(desc,&sea,sizeof(sea));std::memcpy(pixels,sea_pixels,sizeof(pixels));
  for(const void*p:{static_cast<const void*>(&sea),static_cast<const void*>(reinterpret_cast<const unsigned char*>(&sea)+sizeof(sea)-2),static_cast<const void*>(sea_pixels),static_cast<const void*>(sea_pixels+sizeof(sea_pixels)-2)})for(unsigned k=0;k<3;++k){auto address=reinterpret_cast<std::uintptr_t>(p);if(k==1)++address;if(k==2)address-=16383;auto bad=s;bad.pixels=reinterpret_cast<unsigned char*>(address);if(render_world_scene_with_sea_platform(w,r,d,x,y,in,m,c,o,a,sea,bad)!=TerrainSceneStatus::invalid_input||std::memcmp(desc,&sea,sizeof(sea))||std::memcmp(pixels,sea_pixels,sizeof(pixels)))std::exit(21);}
 }
 if(padded_profile&&s.pixels==regular_pixels&&s.bytes==16384&&s.width==128&&s.height==128&&s.pitch==128){unsigned char padded[32768];std::memset(padded,193,sizeof(padded));for(unsigned row=0;row<128;++row)std::memcpy(padded+row*256,s.pixels+row*128,128);auto ps=s;ps.pixels=padded;ps.pitch=256;ps.bytes=32768;const auto result=render_world_scene_with_sea_platform(w,r,d,x,y,in,m,c,o,a,sea,ps);for(unsigned row=0;row<128;++row){for(unsigned col=128;col<256;++col)if(padded[row*256+col]!=193)std::exit(22);if(result==TerrainSceneStatus::rendered)std::memcpy(s.pixels+row*128,padded+row*256,128);}return result;}
 return render_world_scene_with_sea_platform(w,r,d,x,y,in,m,c,o,a,sea,s);}
}
#define render_world_ordinary_scene scene_adapter
#define main parent_main
#include "terrain_scene_probe.cpp"
#undef main
#undef render_world_ordinary_scene
int main(int argc,char**argv){if(argc<4)return 20;FILE*f=std::fopen(argv[argc-3],"rb");if(!f)return 20;const bool ok=std::fread(sea_pixels,1,sizeof(sea_pixels),f)==sizeof(sea_pixels)&&std::fgetc(f)==EOF;std::fclose(f);if(!ok)return 20;sea.minimum_width=std::atoi(argv[argc-2]);sea.minimum_height=std::atoi(argv[argc-1]);if(sea.minimum_width==-2){sea.minimum_width=0;sea.coherent_source_cache_supplied=false;}else if(sea.minimum_width==-3){sea.minimum_width=0;sea.prepared_owner_pixels_supplied=false;}else if(sea.minimum_width==-4){sea.minimum_width=0;sea.pixels.width=71;}else if(sea.minimum_width==-5){sea.minimum_width=0;sea.clip.right=129;}else if(sea.minimum_width==-6){sea.minimum_width=0;padded_profile=true;}return parent_main(argc-3,argv);}
