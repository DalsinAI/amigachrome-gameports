/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently written generic unit assembly; no decompiled source included.
 * Projection/light/scale constants are the ones already qualified for the
 * infantry and speeder profiles (cvr_infantry.cpp).
 */
#include "cvr_vehicle.h"
#include "cvr_lighting.h"
#include "cvr_palette.h"
#include "cvr_raster.h"
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>
namespace alpha { namespace {
constexpr size_t cache_budget=4u*1024u*1024u;
struct Projected { float x,y,z;uint16_t normal;uint8_t color; };
bool inverse_light(const float* m,const float* light,float* out){
 const double det=(double)m[0]*(m[4]*m[8]-m[5]*m[7])-(double)m[1]*(m[3]*m[8]-m[5]*m[6])+(double)m[2]*(m[3]*m[7]-m[4]*m[6]);
 if(!isfinite(det) || fabs(det)<1e-8)return false;
 const double inv[9]={m[4]*m[8]-m[5]*m[7],m[2]*m[7]-m[1]*m[8],m[1]*m[5]-m[2]*m[4],m[5]*m[6]-m[3]*m[8],m[0]*m[8]-m[2]*m[6],m[2]*m[3]-m[0]*m[5],m[3]*m[7]-m[4]*m[6],m[1]*m[6]-m[0]*m[7],m[0]*m[4]-m[1]*m[3]};
 for(unsigned i=0;i<3;++i){out[i]=(float)((inv[i*3]*light[0]+inv[i*3+1]*light[1]+inv[i*3+2]*light[2])/det);if(!isfinite(out[i]))return false;}
 return true;
}
void blit(const uint8_t* rgba,unsigned sw,unsigned sh,uint8_t* output,unsigned width,unsigned height,int cx,int foot,unsigned zoom){
 for(unsigned y=0;y<sh;++y){const int dy=foot-(int)sh+8*(int)zoom+(int)y;if(dy<0 || dy>=(int)height)continue;
  for(unsigned x=0;x<sw;++x){const size_t n=(size_t)y*sw+x;const int dx=cx-(int)sw/2+(int)x;if(rgba[n*4+3] && dx>=0 && dx<(int)width)memcpy(output+((size_t)dy*width+dx)*3,rgba+n*4,3);}}
}
// Heap objects use malloc plus placement new: no libstdc++ allocator or
// exception runtime is linked into the AmigaOS build.
CvrVehicle* create_vehicle(){void* p=malloc(sizeof(CvrVehicle));return p?new(p) CvrVehicle():nullptr;}
void destroy_vehicle(CvrVehicle* v){if(v){v->~CvrVehicle();free(v);}}
bool stem_char(char c){return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';}
void set_recipe(CvrRecipe& r,const char* const* names,const bool* painted,unsigned count,float scale,float root,unsigned frames,unsigned limit){
 memset(&r,0,sizeof(r));r.count=count;
 for(unsigned i=0;i<count;++i){strncpy(r.names[i],names[i],sizeof(r.names[i])-1);r.painted[i]=painted[i];}
 r.profile={scale,root,frames,limit};
}
}
CvrVehicle::CvrVehicle():painted_{},parts_(0),profile_{},lattice_(nullptr),ready_(false),global_(false),global_rgb_{},tints_{},tint_mask_(0),neutral_{},faction_{},cache_{},cache_bytes_(0){}
CvrVehicle::~CvrVehicle(){drop_caches();for(auto*p:neutral_)free(p);for(auto*p:faction_)free(p);free(lattice_);}
void CvrVehicle::drop_caches(){for(auto&row:cache_)for(auto&c:row){free(c);c=nullptr;}cache_bytes_=0;}
bool CvrVehicle::load(const CvrVehiclePart* parts,size_t count,const CvrVehicleProfile& profile){
 ready_=false;drop_caches();
 for(auto&p:neutral_){free(p);p=nullptr;}for(auto&p:faction_){free(p);p=nullptr;}
 parts_=0;
 if(!parts || !count || count>cvr_vehicle_max_parts || !(profile.scale>0) || profile.scale>16 || !isfinite(profile.root_x) || fabsf(profile.root_x)>10000 || !profile.voxel_limit || profile.voxel_limit>400000 || profile.body_frames>256)return false;
 const float normal_scale=.07000000029802322f*profile.scale*(64.0f/100.0f);
 size_t total=0;
 for(unsigned i=0;i<count;++i){
  if(!parts[i].data || !models_[i].load_at_scale(parts[i].data,parts[i].size,normal_scale*2) || models_[i].frame_count()<(i?1u:(profile.body_frames?profile.body_frames:1u)))return false;
  // One geometry snapshot serves both zooms; reject assets whose LOD differs.
  CvrModel low;if(!low.load_at_scale(parts[i].data,parts[i].size,normal_scale) || low.object_count()!=models_[i].object_count())return false;
  for(unsigned j=0;j<low.object_count();++j)if(low.object(j)->units!=models_[i].object(j)->units || low.object(j)->count!=models_[i].object(j)->count)return false;
  total+=models_[i].voxel_count();if(total>profile.voxel_limit)return false;
  painted_[i]=parts[i].painted;
 }
 for(unsigned i=0;i<count;++i){neutral_[i]=(uint8_t*)malloc(6144);if(!neutral_[i])return false;
  for(unsigned s=0;s<24;++s)for(unsigned c=0;c<256;++c)neutral_[i][s*256+c]=models_[i].shaded_index((uint8_t)c,s);}
 if(!lattice_)lattice_=(uint8_t*)malloc(cvr_shade_lattice_size);
 if(!lattice_ || !build_cvr_shade_lattice(lattice_,cvr_shade_lattice_size))return false;
 parts_=(unsigned)count;profile_=profile;ready_=true;return true;
}
bool CvrVehicle::set_colours(const uint8_t* global_rgb,const uint8_t (*tints)[3],uint8_t tint_mask){
 if(tint_mask && (!tints || !global_rgb))return false;
 drop_caches();for(auto&p:faction_){free(p);p=nullptr;}
 global_=global_rgb!=nullptr;if(global_)memcpy(global_rgb_,global_rgb,768);
 tint_mask_=tint_mask;memset(tints_,0,sizeof(tints_));
 if(tints)for(unsigned f=0;f<8;++f)if(tint_mask&(1u<<f))memcpy(tints_[f],tints[f],3);
 return true;
}
const uint8_t* CvrVehicle::table(unsigned part,unsigned faction){
 if(!global_ || !painted_[part] || faction>7 || !(tint_mask_&(1u<<faction)))return neutral_[part];
 if(!faction_[faction]){
  auto* t=(uint8_t*)malloc((size_t)cvr_vehicle_max_parts*6144);if(!t)return nullptr;
  for(unsigned i=0;i<parts_;++i)
   if(painted_[i] && !build_cvr_faction_palette(global_rgb_,768,neutral_[i],6144,tints_[faction],3,t+(size_t)i*6144,6144)){free(t);return nullptr;}
  faction_[faction]=t;
 }
 return faction_[faction]+(size_t)part*6144;
}
bool CvrVehicle::draw(uint8_t* output,size_t capacity,unsigned width,unsigned height,int cx,int foot,unsigned zoom,unsigned facing,unsigned faction){
 if(!ready_ || !output || !width || !height || width>4096 || height>4096 || capacity<(size_t)width*height*3 || zoom<1 || zoom>2 || facing>7 || faction>7 || cx<0 || foot<0 || cx>=(int)width || foot>=(int)height)return false;
 const unsigned sw=128*zoom,sh=144*zoom;const size_t cells=(size_t)sw*sh;
 uint8_t*& cached=cache_[faction][(zoom-1)*8+facing];
 if(cached){blit(cached,sw,sh,output,width,height,cx,foot,zoom);return true;}
 size_t total=0;for(unsigned i=0;i<parts_;++i)total+=models_[i].voxel_count();
 if(total>profile_.voxel_limit)return false;
 const uint8_t* tables[cvr_vehicle_max_parts];
 for(unsigned i=0;i<parts_;++i)if(!(tables[i]=table(i,faction)))return false;
 auto* pixels=(uint8_t*)malloc(cells);auto* depths=(uint16_t*)malloc(cells*sizeof(uint16_t));auto* before=(uint16_t*)malloc(cells*sizeof(uint16_t));
 auto* rgba=(uint8_t*)calloc(cells,4);auto* points=(Projected*)malloc((total?total:1)*sizeof(Projected));
 if(!pixels || !depths || !before || !rgba || !points){free(pixels);free(depths);free(before);free(rgba);free(points);return false;}
 bool ok=true;size_t at=0;float min_x=1e30f,max_x=-1e30f,max_y=-1e30f;
 const float angle=(float)((facing+5)&7)*.7853749990463257f,co=cosf(angle),si=sinf(angle);
 const float scale=.07000000029802322f*profile_.scale*(64.0f/100.0f)*zoom;
 for(unsigned part=0;ok && part<parts_;++part){const CvrModel& model=models_[part];
  for(unsigned i=0;ok && i<model.object_count();++i){const auto*o=model.object(i);const auto&f=o->frames[0];if(f.flags&0x80)continue;
   for(unsigned j=0;j<o->count;++j){const auto&v=o->voxels[j];float local[3]={v.z*o->units,v.x*o->units,v.y*o->units},world[3];
    for(unsigned a=0;a<3;++a){world[a]=f.location[a];for(unsigned b=0;b<3;++b)world[a]+=f.matrix[a*3+b]*local[b];}
    world[0]+=profile_.root_x;const float side=world[0]*co-world[1]*si,along=world[0]*si+world[1]*co;
    Projected p{side,along*.500459611415863f-world[2]*.8657598495483398f,-along*.8657598495483398f-world[2]*.500459611415863f,v.normal,v.color};
    if(!isfinite(p.x) || !isfinite(p.y) || !isfinite(p.z) || fabsf(p.x)>100000 || fabsf(p.y)>100000 || fabsf(p.z)>100000){ok=false;break;}
    points[at++]=p;min_x=fminf(min_x,p.x);max_x=fmaxf(max_x,p.x);max_y=fmaxf(max_y,p.y);
   }
  }
 }
 if(!at)ok=false;
 for(size_t i=0;i<cells;++i)depths[i]=65535;
 const float origin_x=sw*.5f-(min_x+max_x)*.5f*scale,origin_y=(float)sh-8*zoom-max_y*scale;
 CvrRasterSurface surface{pixels,depths,cells,sw,sh,sw};CvrRasterClip clip{0,0,(int32_t)sw-1,(int32_t)sh-1};
 const float view_light=.5773500204086304f;
 const float light[3]={-view_light,(.500459611415863f-.8657598495483398f)*view_light,-(.8657598495483398f+.500459611415863f)*view_light};
 at=0;
 for(unsigned part=0;ok && part<parts_;++part){const CvrModel& model=models_[part];
  for(unsigned i=0;ok && i<model.object_count();++i){const auto*o=model.object(i);const auto&f=o->frames[0];if(f.flags&0x80)continue;
   float pose[9];for(unsigned col=0;col<3;++col){pose[col]=co*f.matrix[col]-si*f.matrix[3+col];pose[3+col]=si*f.matrix[col]+co*f.matrix[3+col];pose[6+col]=f.matrix[6+col];}
   float object_light[3];uint8_t shades[2048];
   if(!inverse_light(pose,light,object_light) || !cvr_shades_for_light(lattice_,cvr_shade_lattice_size,object_light[0],object_light[1],object_light[2],shades,sizeof(shades))){ok=false;break;}
   auto*batch=(CvrRasterPoint*)malloc((size_t)o->count*sizeof(CvrRasterPoint));if(!batch){ok=false;break;}
   for(unsigned j=0;j<o->count;++j){const auto&p=points[at++];const float x=(origin_x+p.x*scale)*256,y=(origin_y+p.y*scale)*256,z=32768+p.z*16;
    if(x<INT32_MIN || x>=2147483648.0f || y<INT32_MIN || y>=2147483648.0f || z<0 || z>=65535){ok=false;break;}
    batch[j]={(int32_t)x,(int32_t)y,(uint32_t)z<<16,p.normal,p.color};}
   memcpy(before,depths,cells*sizeof(uint16_t));
   if(ok)ok=raster_cvr_points(batch,o->count,shades,sizeof(shades),tables[part],6144,clip,scale*o->units>.5f,surface);
   free(batch);
   if(ok)for(size_t j=0;j<cells;++j)if(depths[j]!=before[j]){memcpy(rgba+j*4,global_?global_rgb_+3*pixels[j]:model.palette_rgb(pixels[j]),3);rgba[j*4+3]=1;}
  }
 }
 // Output changes only after every part, transform and raster batch passed.
 if(ok){
  blit(rgba,sw,sh,output,width,height,cx,foot,zoom);
  if(cache_bytes_+cells*4>cache_budget)drop_caches();
  cached=rgba;rgba=nullptr;cache_bytes_+=cells*4;
 }
 free(pixels);free(depths);free(before);free(rgba);free(points);return ok;
}

bool cvr_builtin_recipe(int chassis,int weapon,int armor,int reactor,uint32_t abilities,CvrRecipe& out){
 if(abilities || weapon!=0 || armor!=0 || reactor!=1)return false;
 if(chassis==0){static const char* const n[]={"vi","viptr00"};static const bool p[]={false,false};set_recipe(out,n,p,2,1.25f,-400,15,100000);return true;}
 if(chassis==1){
  // Speeder append order and its two faction-remapped parts (VGMCP, VBP).
  static const char* const n[]={"vrcp00","vgmc","vgmcp","vb","vbp","vsp","vsptb","vsptf","vlights","vr00"};
  static const bool p[]={false,false,true,false,true,false,false,false,false,false};
  set_recipe(out,n,p,10,1.0f,0,1,160000);return true;
 }
 return false;
}
bool parse_cvr_recipe_table(const char* text,size_t size,CvrRecipeTable& out){
 if(!text || size>64*1024)return false;
 auto* table=(CvrRecipeTable*)calloc(1,sizeof(CvrRecipeTable));if(!table)return false;
 size_t at=0;bool ok=true;
 while(ok && at<size){
  size_t end=at;while(end<size && text[end]!='\n')++end;
  char line[512];size_t n=end-at;if(n && text[at+n-1]=='\r')--n;
  if(n>=sizeof(line)){ok=false;break;}
  memcpy(line,text+at,n);line[n]=0;at=end<size?end+1:end;
  for(size_t i=0;i<n;++i){if(line[i]=='#'){line[i]=0;break;}if((unsigned char)line[i]<32 && line[i]!='\t'){ok=false;break;}}
  if(!ok)break;
  char* p=line;while(*p==' ' || *p=='\t')++p;if(!*p)continue;
  if(table->count>=64){ok=false;break;}
  auto& e=table->entries[table->count];int ints[4];float reals[2];long limits[2];
  for(int k=0;k<4 && ok;++k){char* q;long v=strtol(p,&q,10);if(q==p || v<0 || v>255)ok=false;ints[k]=(int)v;p=q;}
  for(int k=0;k<2 && ok;++k){char* q;float v=strtof(p,&q);if(q==p || !isfinite(v))ok=false;reals[k]=v;p=q;}
  for(int k=0;k<2 && ok;++k){char* q;long v=strtol(p,&q,10);if(q==p || v<0 || v>400000)ok=false;limits[k]=v;p=q;}
  if(!ok)break;
  while(*p==' ' || *p=='\t')++p;
  if(*p!=':'){ok=false;break;}
  ++p;
  CvrRecipe r{};
  for(;;){while(*p==' ' || *p=='\t')++p;if(!*p)break;
   if(r.count>=cvr_vehicle_max_parts){ok=false;break;}
   unsigned len=0;while(stem_char(p[len]))++len;
   if(!len || len>20){ok=false;break;}
   memcpy(r.names[r.count],p,len);r.names[r.count][len]=0;p+=len;
   if(*p=='*'){r.painted[r.count]=true;++p;}
   if(*p && *p!=' ' && *p!='\t'){ok=false;break;}
   ++r.count;
  }
  if(!ok || !r.count || !(reals[0]>0) || reals[0]>16 || fabsf(reals[1])>10000 || limits[0]>256 || !limits[1]){ok=false;break;}
  r.profile={reals[0],reals[1],(unsigned)limits[0],(unsigned)limits[1]};
  for(unsigned k=0;k<table->count;++k){const auto& o=table->entries[k];if(o.chassis==ints[0] && o.weapon==ints[1] && o.armor==ints[2] && o.reactor==ints[3])ok=false;}
  if(!ok)break;
  e.chassis=ints[0];e.weapon=ints[1];e.armor=ints[2];e.reactor=ints[3];e.recipe=r;++table->count;
 }
 if(ok)memcpy(&out,table,sizeof(out));
 free(table);return ok;
}
bool find_cvr_recipe(const CvrRecipeTable* table,int chassis,int weapon,int armor,int reactor,uint32_t abilities,CvrRecipe& out){
 if(table && !abilities)for(unsigned i=0;i<table->count && i<64;++i){const auto& e=table->entries[i];
  if(e.chassis==chassis && e.weapon==weapon && e.armor==armor && e.reactor==reactor){out=e.recipe;return true;}}
 return cvr_builtin_recipe(chassis,weapon,armor,reactor,abilities,out);
}
bool cvr_faction_tint(const uint8_t* indices,uint32_t width,uint32_t height,const uint8_t* global_rgb,uint8_t tint[3]){
 if(!indices || !global_rgb || !tint || width<436 || height<758 || width>4096 || height>4096)return false;
 const uint8_t intensity=indices[(size_t)757*width+161],raw=indices[(size_t)745*width+435];
 if(intensity==255 || raw==255)return false;
 memcpy(tint,global_rgb+3*(uint8_t)(raw+10),3);return true;
}

CvrVehicleSet::CvrVehicleSet():slots_{},failed_{},failed_count_(0),clock_(0),reader_(nullptr),context_(nullptr),table_(nullptr),global_(false),global_rgb_{},tints_{},mask_(0){}
CvrVehicleSet::~CvrVehicleSet(){clear();}
void CvrVehicleSet::clear(){for(auto&s:slots_){destroy_vehicle(s.vehicle);s.vehicle=nullptr;}failed_count_=0;clock_=0;}
void CvrVehicleSet::configure(Reader reader,void* context,const CvrRecipeTable* table,const uint8_t* global_rgb,const uint8_t (*tints)[3],uint8_t mask){
 clear();reader_=reader;context_=context;table_=table;global_=global_rgb!=nullptr;
 if(global_)memcpy(global_rgb_,global_rgb,768);
 mask_=global_ && tints?mask:0;memset(tints_,0,sizeof(tints_));if(tints)memcpy(tints_,tints,sizeof(tints_));
}
CvrVehicle* CvrVehicleSet::get(int chassis,int weapon,int armor,int reactor,uint32_t abilities){
 const int32_t key[5]={chassis,weapon,armor,reactor,(int32_t)abilities};
 ++clock_;
 for(auto&s:slots_)if(s.vehicle && !memcmp(s.key,key,sizeof(key))){s.used=clock_;return s.vehicle;}
 for(unsigned i=0;i<failed_count_;++i)if(!memcmp(failed_[i],key,sizeof(key)))return nullptr;
 CvrRecipe recipe;bool ok=reader_ && find_cvr_recipe(table_,chassis,weapon,armor,reactor,abilities,recipe);
 CvrVehicle* vehicle=nullptr;
 if(ok){
  CvrVehiclePart parts[cvr_vehicle_max_parts]{};uint8_t* owned[cvr_vehicle_max_parts]{};
  for(unsigned i=0;ok && i<recipe.count;++i){char name[32];memcpy(name,recipe.names[i],sizeof(recipe.names[i]));name[23]=0;strcat(name,".cvr");
   size_t n=0;owned[i]=reader_(context_,name,n);if(!owned[i]){ok=false;break;}parts[i]={owned[i],n,recipe.painted[i]};}
  if(ok){vehicle=create_vehicle();ok=vehicle && vehicle->load(parts,recipe.count,recipe.profile) && vehicle->set_colours(global_?global_rgb_:nullptr,mask_?tints_:nullptr,mask_);}
  for(auto*p:owned)free(p);
 }
 if(!ok){destroy_vehicle(vehicle);if(failed_count_<32)memcpy(failed_[failed_count_++],key,sizeof(key));return nullptr;}
 Slot* victim=&slots_[0];
 for(auto&s:slots_){if(!s.vehicle){victim=&s;break;}if(s.used<victim->used)victim=&s;}
 destroy_vehicle(victim->vehicle);memcpy(victim->key,key,sizeof(key));victim->vehicle=vehicle;victim->used=clock_;return vehicle;
}
}
