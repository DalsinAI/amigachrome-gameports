/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "world_fresh_tile_yields.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace alpha;
alignas(TileYieldContext) static Rules technologies;
alignas(TileYieldContext) static BasicRules basic;
alignas(TileYieldContext) static ResourceInfo resources;
static WorldRules world_rules;
static Rules invalid_technologies;
static BasicRules invalid_basic;
alignas(TileYieldContext) static OriginalWorld world;
static unsigned char* read(const char* path,size_t& n){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long z=ftell(f);rewind(f);
 if(z<=0 || (size_t)z>64u*1024u*1024u){fclose(f);return nullptr;}auto* b=(unsigned char*)malloc((size_t)z);if(!b){fclose(f);return nullptr;}bool ok=fread(b,1,(size_t)z,f)==(size_t)z;fclose(f);if(!ok){free(b);return nullptr;}n=(size_t)z;return b;
}
static uint32_t bytes(uint32_t h,const void* p,size_t n){const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
static uint32_t fingerprint(){
 uint32_t h=bytes(2166136261u,&world,sizeof world);const auto& g=world.game();h=bytes(h,world.original_bytes(),world.original_size());
 h=bytes(h,world.tile(0),(size_t)g.map.info.tile_count*sizeof(OriginalMapTile));
 if(g.vehicle_count)h=bytes(h,world.vehicle(0),(size_t)g.vehicle_count*sizeof(OriginalVehicle));
 if(g.base_count)h=bytes(h,world.base(0),(size_t)g.base_count*sizeof(OriginalBase));
 h=bytes(h,world.prototype(0),512*sizeof(OriginalPrototype));h=bytes(h,world.faction(0),8*sizeof(OriginalFactionState));h=bytes(h,world.faction_definition(0),8*sizeof(OriginalFactionDefinition));
 h=bytes(h,&technologies,sizeof technologies);h=bytes(h,&basic,sizeof basic);return bytes(h,&resources,sizeof resources);
}
static void context_words(const TileYieldContext& c,uint32_t* v){
 v[0]=(uint32_t)c.variant;v[1]=c.tile.climate;v[2]=c.tile.val2;v[3]=c.tile.val3;v[4]=c.tile.items;v[5]=c.tile.landmarks;v[6]=c.bonus;v[7]=c.expansion_enabled;v[8]=c.terraform_preview;v[9]=(uint32_t)c.planet_pending;v[10]=(uint32_t)c.economy_pending;
 for(unsigned i=0;i<3;++i)v[11+i]=(uint32_t)c.fungus[i];
 v[14]=(uint32_t)c.base_count;v[15]=c.golden_age;v[16]=c.manifold_owned;v[17]=c.aquatic;v[18]=(uint32_t)c.facilities;v[19]=(uint32_t)(c.facilities>>32);v[20]=0;
 for(unsigned i=0;i<3;++i)if(c.allow_three[i])v[20]|=1u<<i;
 if(c.sea_mineral_technology)v[20]|=8;
 v[21]=c.governor;v[22]=c.merchant_exchange;v[23]=c.dust_cloud;v[24]=c.solar_flares;v[25]=c.control_upkeep;v[26]=c.echelon_matches;v[27]=(uint32_t)c.mine_nutrient_effect;v[28]=(uint32_t)c.mine_without_road_limit;v[29]=c.base_event_flags;v[30]=(uint32_t)c.terraform_reduction_before;v[31]=(uint32_t)c.base_energy_before;
 for(unsigned i=0;i<9;++i)for(unsigned j=0;j<3;++j)v[32+i*3+j]=(uint32_t)c.resources[i][j];
}
static bool rejected(const Rules& rules,const BasicRules& basics,const WorldTileYieldStage& stage){
 TileYieldContext context;memset(&context,0x5a,sizeof context);TileYieldContext old_context;memcpy(&old_context,&context,sizeof context);
 TileYieldResult result;memset(&result,0x5a,sizeof result);TileYieldResult old_result;memcpy(&old_result,&result,sizeof result);
 return world_fresh_tile_yield_context(world,rules,basics,resources,0,5,0,0,stage,context)==TileYieldError::bad_state &&
  world_fresh_tile_yields(world,rules,basics,resources,0,5,0,0,stage,result)==TileYieldError::bad_state &&
  !memcmp(&context,&old_context,sizeof context) && !memcmp(&result,&old_result,sizeof result);
}
static bool emit(const uint32_t* fields,size_t n){unsigned char b[4];for(size_t i=0;i<n;++i){for(unsigned j=0;j<4;++j)b[j]=(unsigned char)(fields[i]>>(8*j));if(fwrite(b,1,4,stdout)!=4)return false;}return true;}
int main(int argc,char** argv){
 if(argc!=4 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion")))return 20;
 const bool base_profile=!strcmp(argv[1],"base");const Variant variant=base_profile ? Variant::base : Variant::alien_crossfire;
 size_t n=0;auto* text=read(argv[2],n);if(!text)return 20;
 bool ok=read_technologies(text,n,technologies)==Error::ok && read_basic_rules(text,n,technologies,variant,basic,world_rules)==Error::ok && read_resource_info(text,n,resources)==Error::ok;memset(text,0xff,n);free(text);if(!ok)return 20;
 auto* save=read(argv[3],n);if(!save)return 20;const auto loaded=world.load(save,n,0xabcdef01);memset(save,0xff,n);free(save);if(loaded!=WorldLoadError::ok)return 20;
 const uint32_t before=fingerprint();alignas(TileYieldContext) WorldTileYieldStage stage{variant,false,false,false,-1,0,-123456};
 if(world.game().base_count){
  struct Span {const void* p;size_t n;};
  const auto& g=world.game();const Span spans[]={
   {&world,sizeof world},{world.original_bytes(),world.original_size()},
   {world.tile(0),g.map.info.tile_count*sizeof(OriginalMapTile)},
   {world.vehicle(0),g.vehicle_count*sizeof(OriginalVehicle)},
   {world.prototype(0),512*sizeof(OriginalPrototype)},{world.base(0),g.base_count*sizeof(OriginalBase)},
   {world.faction(0),8*sizeof(OriginalFactionState)},{world.faction_definition(0),8*sizeof(OriginalFactionDefinition)},
   {&technologies,sizeof technologies},{&basic,sizeof basic},{&resources,sizeof resources},{&stage,sizeof stage}};
  for(const auto& span:spans){if(!span.p||!span.n)continue;
   const size_t offsets[]={0,(span.n/2)&~(size_t)(alignof(TileYieldContext)-1),(span.n-1)&~(size_t)(alignof(TileYieldContext)-1)};
   for(size_t off:offsets){const void* p=(const unsigned char*)span.p+off;
    WorldTileYieldStage old_stage;memcpy(&old_stage,&stage,sizeof stage);
    auto& c=*reinterpret_cast<TileYieldContext*>(const_cast<void*>(p));
    if(world_fresh_tile_yield_context(world,technologies,basic,resources,0,5,0,0,stage,c)!=TileYieldError::bad_buffer)return 21;
    auto& r=*reinterpret_cast<TileYieldResult*>(const_cast<void*>(p));
    if(world_fresh_tile_yields(world,technologies,basic,resources,0,5,0,0,stage,r)!=TileYieldError::bad_buffer)return 21;
    if(fingerprint()!=before || memcmp(&stage,&old_stage,sizeof stage))return 21;
   }
  }
 }
 WorldTileYieldStage invalid_stage=stage;invalid_stage.variant=(Variant)99;
 if(!rejected(technologies,basic,invalid_stage))return 21;
 invalid_stage=stage;invalid_stage.variant=Variant::base;invalid_stage.expansion_enabled=true;
 if(!rejected(technologies,basic,invalid_stage))return 21;
 invalid_technologies=technologies;invalid_technologies.count=88;
 if(!rejected(invalid_technologies,basic,stage))return 21;
 invalid_technologies=technologies;invalid_technologies.technologies[88].prerequisites[1]=89;
 if(!rejected(invalid_technologies,basic,stage))return 21;
 invalid_basic=basic;invalid_basic.limit_mineral_mine_sans_road=-1;
 if(!rejected(technologies,invalid_basic,stage))return 21;
 const int32_t width=(int32_t)world.game().map.info.width,height=(int32_t)world.game().map.info.height;
 const int32_t xs[]={0,1,width-2,width-1,-1,1,width},ys[]={0,1,0,(height-1)|1,0,0,0};
 for(size_t b=0;b<=world.game().base_count;++b)for(int32_t f=-1;f<=8;++f)for(unsigned position=0;position<7;++position)for(unsigned mode=0;mode<4;++mode){
  stage={variant,!base_profile && (mode&1u),mode>=2,(mode&1u)!=0,mode==0 ? (int32_t)b : mode==1 ? 1-(int32_t)b : mode==2 ? -1 : 999,(int32_t)mode-2,-123456};
  TileYieldContext context;memset(&context,0x5a,sizeof context);TileYieldContext old_context;memcpy(&old_context,&context,sizeof context);
  TileYieldResult result;memset(&result,0x5a,sizeof result);TileYieldResult old_result;memcpy(&old_result,&result,sizeof result);
  const auto c=world_fresh_tile_yield_context(world,technologies,basic,resources,b,f,xs[position],ys[position],stage,context);
  const auto r=world_fresh_tile_yields(world,technologies,basic,resources,b,f,xs[position],ys[position],stage,result);
  if(c!=TileYieldError::ok && (memcmp(&context,&old_context,sizeof context) || r!=c))return 21;
  if(r!=TileYieldError::ok && memcmp(&result,&old_result,sizeof result))return 21;
  uint32_t fields[70];for(auto& value:fields)value=0x5a5a5a5a;fields[0]=(uint32_t)c;fields[60]=(uint32_t)r;
  if(c==TileYieldError::ok)context_words(context,fields+1);
  if(r==TileYieldError::ok){const int32_t values[]={result.nutrient,result.mineral,result.energy,result.terraform_reduction,result.base_energy,result.reduction_by_resource[0],result.reduction_by_resource[1],result.reduction_by_resource[2],result.base_energy_written ? 1 : 0};for(unsigned i=0;i<9;++i)fields[61+i]=(uint32_t)values[i];}
  if(!emit(fields,70))return 20;
 }
 return fingerprint()==before && world.runtime_seed()==0xabcdef01 ? 0 : 21;
}
