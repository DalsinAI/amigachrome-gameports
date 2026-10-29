/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic-fixture control for cvr_vehicle; prints digests for the Python oracle.
 */
#include "cvr_vehicle.h"
#include "cvr_infantry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
namespace {
uint8_t* read(const char* path,size_t& size){FILE*f=fopen(path,"rb");if(!f)return nullptr;fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<=0 || n>16*1024*1024){fclose(f);return nullptr;}auto*b=(uint8_t*)malloc(n);if(!b || fread(b,1,n,f)!=(size_t)n){free(b);fclose(f);return nullptr;}fclose(f);size=(size_t)n;return b;}
uint32_t fnv(const uint8_t* p,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
const size_t W=256,H=256,BYTES=W*H*3;
struct Parts{uint8_t* data[12];alpha::CvrVehiclePart parts[12];unsigned count;
 ~Parts(){for(unsigned i=0;i<count;++i)free(data[i]);}};
bool load_parts(const char* dir,unsigned count,unsigned mask,Parts& out){
 out.count=0;if(count>12)return false;
 for(unsigned i=0;i<count;++i){char path[1024];snprintf(path,sizeof(path),"%s/part%u.cvr",dir,i);size_t n=0;out.data[i]=read(path,n);out.count=i+1;if(!out.data[i])return false;out.parts[i]={out.data[i],n,(mask>>i&1)!=0};}
 return true;
}
/* Draw every zoom/facing; print counts of the background and of each RGB. */
int report(alpha::CvrVehicle& v,unsigned faction){
 auto*rgb=(uint8_t*)malloc(BYTES);if(!rgb)return 20;
 for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){
  memset(rgb,19,BYTES);if(!v.draw(rgb,BYTES,W,H,128,200,zoom,facing,faction)){for(size_t i=0;i<BYTES;++i)if(rgb[i]!=19){free(rgb);return 20;}free(rgb);puts("DRAW rejected");return 0;}
  unsigned changed=0;uint32_t colours[8][4]={};unsigned distinct=0;
  for(size_t i=0;i<BYTES;i+=3){if(rgb[i]==19 && rgb[i+1]==19 && rgb[i+2]==19)continue;++changed;
   unsigned k=0;for(;k<distinct;++k)if(colours[k][0]==rgb[i] && colours[k][1]==rgb[i+1] && colours[k][2]==rgb[i+2])break;
   if(k==distinct){if(distinct==8){free(rgb);return 20;}colours[k][0]=rgb[i];colours[k][1]=rgb[i+1];colours[k][2]=rgb[i+2];++distinct;}
   ++colours[k][3];}
  printf("VIEW zoom=%u facing=%u changed=%u digest=%08x",zoom,facing,changed,fnv(rgb,BYTES));
  for(unsigned k=0;k<distinct;++k)printf(" %u,%u,%u=%u",colours[k][0],colours[k][1],colours[k][2],colours[k][3]);
  puts("");
 }
 // Cached redraw over a different background must reproduce only the sprite.
 memset(rgb,47,BYTES);if(!v.draw(rgb,BYTES,W,H,128,200,1,3,faction)){free(rgb);return 20;}
 printf("CACHED digest=%08x bytes=%lu\n",fnv(rgb,BYTES),(unsigned long)v.cache_bytes());
 struct Bad{size_t capacity;unsigned w,h,zoom,facing,faction;int x,y;};
 const Bad invalid[]={{BYTES-1,W,H,1,0,0,128,200},{BYTES,W,H,0,0,0,128,200},{BYTES,W,H,3,0,0,128,200},{BYTES,W,H,1,8,0,128,200},{BYTES,W,H,1,0,8,128,200},{BYTES,0,H,1,0,0,128,200},{BYTES,W,H,1,0,0,-1,200},{BYTES,W,H,1,0,0,128,256}};
 unsigned rejected=0;
 for(const auto&b:invalid){memset(rgb,19,BYTES);if(v.draw(rgb,b.capacity,b.w,b.h,b.x,b.y,b.zoom,b.facing,b.faction)){free(rgb);return 20;}for(size_t i=0;i<BYTES;++i)if(rgb[i]!=19){free(rgb);return 20;}++rejected;}
 printf("INVALID rejected=%u\n",rejected);free(rgb);return 0;
}
}
int main(int argc,char**argv){
 if(argc<2)return 20;
 const char* mode=argv[1];
 if(!strcmp(mode,"assemble") && argc==12){
  // DIR COUNT MASK SCALE ROOT FRAMES LIMIT GLOBAL|- TINTS|- FACTION
  Parts p{};unsigned count=(unsigned)atoi(argv[3]),mask=(unsigned)strtoul(argv[4],nullptr,0);
  const alpha::CvrVehicleProfile profile{(float)atof(argv[5]),(float)atof(argv[6]),(unsigned)atoi(argv[7]),(unsigned)atoi(argv[8])};
  alpha::CvrVehicle v;
  const bool loaded=load_parts(argv[2],count,mask,p) && v.load(p.parts,count,profile);
  if(!loaded){puts("LOAD rejected");uint8_t rgb[3*16*16];memset(rgb,19,sizeof(rgb));if(v.draw(rgb,sizeof(rgb),16,16,8,8,1,0,0))return 20;for(auto c:rgb)if(c!=19)return 20;return 0;}
  size_t gn=0,tn=0;uint8_t* global=strcmp(argv[9],"-")?read(argv[9],gn):nullptr;uint8_t* tints=strcmp(argv[10],"-")?read(argv[10],tn):nullptr;
  if((global && gn!=768) || (tints && tn!=25)){free(global);free(tints);return 20;}
  if(!v.set_colours(global,tints?(const uint8_t(*)[3])tints:nullptr,tints?tints[24]:0)){free(global);free(tints);return 20;}
  // Rejected colour changes leave the configured colours in place.
  if(v.set_colours(nullptr,nullptr,1)){free(global);free(tints);return 20;}
  if(!v.set_colours(global,tints?(const uint8_t(*)[3])tints:nullptr,tints?tints[24]:0)){free(global);free(tints);return 20;}
  free(global);free(tints);
  printf("LOAD parts=%u\n",v.parts());return report(v,(unsigned)atoi(argv[11]));
 }
 if(!strcmp(mode,"equiv") && argc==4){
  // DIR infantry|speeder: model-colour CvrVehicle must equal CvrUnitAssembly byte-for-byte.
  const bool speeder=!strcmp(argv[3],"speeder");const unsigned count=speeder?10:2;
  Parts p{};if(!load_parts(argv[2],count,0,p))return 20;
  alpha::CvrRecipe recipe;if(!alpha::cvr_builtin_recipe(speeder?1:0,0,0,1,0,recipe) || recipe.count!=count)return 20;
  alpha::CvrVehicle v;if(!v.load(p.parts,count,recipe.profile))return 20;
  alpha::CvrUnitAssembly old;alpha::CvrPartInput in[10];for(unsigned i=0;i<count;++i)in[i]={p.parts[i].data,p.parts[i].size};
  if(!(speeder?old.load_speeder(in,10):old.load(in[0].data,in[0].size,in[1].data,in[1].size)))return 20;
  auto*a=(uint8_t*)malloc(BYTES);auto*b=(uint8_t*)malloc(BYTES);if(!a || !b)return 20;unsigned same=0;
  for(unsigned pass=0;pass<2;++pass)for(unsigned zoom=1;zoom<=2;++zoom)for(unsigned facing=0;facing<8;++facing){
   memset(a,19+pass,BYTES);memset(b,19+pass,BYTES);
   if(!v.draw(a,BYTES,W,H,128,200,zoom,facing) || !old.draw(b,BYTES,W,H,128,200,zoom,facing) || memcmp(a,b,BYTES)){free(a);free(b);printf("EQUIV mismatch zoom=%u facing=%u pass=%u\n",zoom,facing,pass);return 20;}
   ++same;}
  free(a);free(b);printf("EQUIV %s views=%u identical\n",argv[3],same);return 0;
 }
 if(!strcmp(mode,"recipes") && argc==3){
  size_t n=0;uint8_t* text=read(argv[2],n);alpha::CvrRecipeTable* t=(alpha::CvrRecipeTable*)calloc(1,sizeof(alpha::CvrRecipeTable));if(!t){free(text);return 20;}
  t->count=77;const bool ok=text && alpha::parse_cvr_recipe_table((const char*)text,n,*t);free(text);
  if(!ok){printf("RECIPES rejected unchanged=%d\n",t->count==77);free(t);return 0;}
  printf("RECIPES count=%u",t->count);
  for(unsigned i=0;i<t->count;++i){const auto&e=t->entries[i];printf(" [%d/%d/%d/%d %g %g %u %u:",e.chassis,e.weapon,e.armor,e.reactor,(double)e.recipe.profile.scale,(double)e.recipe.profile.root_x,e.recipe.profile.body_frames,e.recipe.profile.voxel_limit);
   for(unsigned k=0;k<e.recipe.count;++k)printf(" %s%s",e.recipe.names[k],e.recipe.painted[k]?"*":"");
   printf("]");}
  puts("");
  alpha::CvrRecipe r;
  printf("LOOKUP table=%d builtin_inf=%d builtin_speeder=%d abilities=%d none=%d\n",t->count?(int)alpha::find_cvr_recipe(t,t->entries[0].chassis,t->entries[0].weapon,t->entries[0].armor,t->entries[0].reactor,0,r):-1,
   (int)alpha::find_cvr_recipe(t,0,0,0,1,0,r),(int)alpha::find_cvr_recipe(t,1,0,0,1,0,r),(int)alpha::find_cvr_recipe(t,0,0,0,1,4,r),(int)alpha::find_cvr_recipe(nullptr,9,9,9,9,0,r));
  free(t);return 0;
 }
 if(!strcmp(mode,"tint") && argc==6){
  // INDICES WIDTH HEIGHT GLOBAL
  size_t n=0,gn=0;uint8_t* idx=read(argv[2],n);uint8_t* global=read(argv[5],gn);const uint32_t w=(uint32_t)atoi(argv[3]),h=(uint32_t)atoi(argv[4]);
  if(!idx || !global || gn!=768 || n!=(size_t)w*h){free(idx);free(global);return 20;}
  uint8_t tint[3]={7,7,7};const bool ok=alpha::cvr_faction_tint(idx,w,h,global,tint);
  printf("TINT ok=%d rgb=%u,%u,%u\n",ok,tint[0],tint[1],tint[2]);free(idx);free(global);return 0;
 }
 if(!strcmp(mode,"set") && argc==3){
  // Directory reader: infantry via the built-in recipe, a missing speeder, eviction.
  struct Ctx{const char* dir;unsigned reads;} ctx{argv[2],0};
  auto reader=[](void* c,const char* name,size_t& n)->uint8_t*{auto*x=(Ctx*)c;char path[1024];snprintf(path,sizeof(path),"%s/%s",x->dir,name);++x->reads;return read(path,n);};
  size_t tn=0;char tpath[1024];snprintf(tpath,sizeof(tpath),"%s/cvr_units.txt",argv[2]);uint8_t* table_text=read(tpath,tn);
  auto* table=(alpha::CvrRecipeTable*)calloc(1,sizeof(alpha::CvrRecipeTable));if(!table || !table_text || !alpha::parse_cvr_recipe_table((const char*)table_text,tn,*table)){free(table);free(table_text);return 20;}
  free(table_text);
  alpha::CvrVehicleSet set;set.configure(reader,&ctx,table,nullptr,nullptr,0);
  alpha::CvrVehicle* a=set.get(0,0,0,1,0);const unsigned after_first=ctx.reads;alpha::CvrVehicle* b=set.get(0,0,0,1,0);
  alpha::CvrVehicle* missing=set.get(1,0,0,1,0);const unsigned after_missing=ctx.reads;alpha::CvrVehicle* again=set.get(1,0,0,1,0);const bool memo=again==nullptr && ctx.reads==after_missing;
  alpha::CvrVehicle* ability=set.get(0,0,0,1,8);
  // Seven distinct table designs through six slots: the first is evicted and reloaded.
  bool all=true;for(int k=0;k<7;++k)all=all && set.get(20+k,0,0,1,0);
  const unsigned before_reload=ctx.reads;alpha::CvrVehicle* reloaded=set.get(20,0,0,1,0);
  printf("SET first=%d same=%d reads=%u missing=%d memo=%d ability=%d table=%d evicted_reload=%d\n",a!=nullptr,a==b,after_first,missing!=nullptr,memo,ability!=nullptr,all,reloaded!=nullptr && ctx.reads>before_reload);
  set.clear();free(table);return 0;
 }
 return 20;
}
