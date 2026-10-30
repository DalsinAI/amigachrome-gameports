/* SPDX-License-Identifier: GPL-3.0-or-later
 * asmac view: the native map window's drawing (original_map_window.cpp
 * draw(): flat texture.pcx terrain diamonds, rocky overlay, MapViewExtras
 * overlays/fog/borders, faction base sprites, Units.pcx native-life sprites,
 * assembled CVR vehicles, the infantry/speeder assemblies), the minimap, the
 * base window and the console.pcx HUD art, into a caller's RGB24 frame.
 *
 * The native window keeps these steps in static functions of an AmigaOS-only
 * translation unit, so this file repeats the same steps over the same
 * modules (PcxImage, MapViewExtras, map_overlays, ui_minimap, cvr_*,
 * world_unit_stats) for a host frontend; it adds no rule and changes no
 * native file. The view is rebuilt from the session's SAV bytes with
 * OriginalWorld exactly as play_rebuild() does. Assets are the user's own
 * files, read from the data directory given to asmac_view_open.
 */
#include "asmac.h"
#include "play_session.h"
#include "original_world.h"
#include "pcx_image.h"
#include "ui_mapview.h"
#include "cvr_infantry.h"
#include "rules_text.h"
#include "world_unit_stats.h"
#include "audio_events.h"
#include <dirent.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <vector>

namespace P=alpha::play;

struct asmac_view {
 std::string dir;bool expansion=false;
 alpha::PcxImage terrain,sprites,console,faction[8];
 bool terrain_ok=false,sprites_ok=false,console_ok=false;unsigned faction_mask=0;
 alpha::OriginalWorld world;bool world_ok=false;
 alpha::MapViewExtras extras;bool extras_open=false;
 alpha::CvrUnitAssembly infantry,speeder;bool infantry_ok=false,speeder_ok=false;
 alpha::Rules* technologies=nullptr;alpha::UnitDefinitions* definitions=nullptr;alpha::FacilityDefinitions* facilities=nullptr;
 bool rules_ok=false,facilities_ok=false;
 ~asmac_view(){extras.close();delete technologies;delete definitions;delete facilities;}
};

/* Case-insensitive file lookup in the data directory (the user's install may
 * be on a case-sensitive file system with any spelling). */
static bool find_file(const std::string& dir,const char* name,std::string& out){
 std::string sub;const char* base=name;const char* slash=strrchr(name,'/');
 std::string d=dir.empty()?".":dir;
 if(slash){sub.assign(name,(size_t)(slash-name));base=slash+1;std::string found;
  if(!find_file(dir,sub.c_str(),found))return false;d=found;}
 std::string direct=d+"/"+base;FILE* f=fopen(direct.c_str(),"rb");if(f){fclose(f);out=direct;return true;}
 DIR* dd=opendir(d.c_str());if(!dd)return false;bool ok=false;
 while(dirent* e=readdir(dd)){if(!strcasecmp(e->d_name,base)){out=d+"/"+e->d_name;ok=true;break;}}
 closedir(dd);return ok;
}
static uint8_t* read_all(const std::string& path,size_t& size){
 FILE* f=fopen(path.c_str(),"rb");if(!f)return nullptr;
 if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);
 if(n<=0 || n>64*1024*1024){fclose(f);return nullptr;}
 auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}
 const bool ok=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(p);return nullptr;}size=(size_t)n;return p;
}
static uint8_t* reader(void* ctx,const char* name,size_t& size){
 const asmac_view* v=(const asmac_view*)ctx;std::string path;if(!find_file(v->dir,name,path))return nullptr;return read_all(path,size);
}
static bool load_pcx(const asmac_view* v,const char* name,alpha::PcxImage& img,unsigned w,unsigned h){
 size_t n=0;uint8_t* p=reader((void*)v,name,n);if(!p)return false;
 const bool ok=img.load(p,n) && (!w || (img.width()==w && img.height()==h));free(p);return ok;
}

asmac_view* asmac_view_open(const char* dir,int expansion){
 auto* v=new(std::nothrow) asmac_view();if(!v)return nullptr;
 v->dir=dir && dir[0]?dir:".";v->expansion=expansion!=0;
 v->terrain_ok=load_pcx(v,"texture.pcx",v->terrain,1024,768);
 v->sprites_ok=load_pcx(v,"Units.pcx",v->sprites,1024,768);
 v->console_ok=load_pcx(v,"console.pcx",v->console,800,257);
 // Optional original models (native: VI.cvr + Viptr00.cvr; the speeder parts).
 size_t bs=0,rs=0;uint8_t* body=reader(v,"VI.cvr",bs);uint8_t* reactor=reader(v,"Viptr00.cvr",rs);
 v->infantry_ok=body && reactor && v->infantry.load(body,bs,reactor,rs);free(body);free(reactor);
 const char* names[]={"vrcp00.cvr","vgmc.cvr","vgmcp.cvr","vb.cvr","vbp.cvr","vsp.cvr","vsptb.cvr","vsptf.cvr","vlights.cvr","vr00.cvr"};
 alpha::CvrPartInput parts[10]{};uint8_t* owned[10]{};bool all=true;
 for(unsigned i=0;i<10;++i){owned[i]=reader(v,names[i],parts[i].size);parts[i].data=owned[i];all=all && owned[i];}
 v->speeder_ok=all && v->speeder.load_speeder(parts,10);for(auto* p:owned)free(p);
 // Rules tables for unit morale (sprite column) and facility names.
 size_t n=0;uint8_t* rules=reader(v,v->expansion?"alphax.txt":"alpha.txt",n);
 if(rules){
  v->technologies=new(std::nothrow) alpha::Rules();v->definitions=new(std::nothrow) alpha::UnitDefinitions();v->facilities=new(std::nothrow) alpha::FacilityDefinitions();
  const auto variant=v->expansion?alpha::Variant::alien_crossfire:alpha::Variant::base;
  if(v->technologies && v->definitions && alpha::read_technologies(rules,n,*v->technologies)==alpha::Error::ok){
   v->rules_ok=alpha::read_unit_definitions(rules,n,*v->technologies,variant,*v->definitions)==alpha::Error::ok;
   v->facilities_ok=v->facilities && alpha::read_facility_definitions(rules,n,*v->technologies,variant,*v->facilities)==alpha::Error::ok;}
  free(rules);}
 return v;
}
void asmac_view_free(asmac_view* v){delete v;}
unsigned asmac_view_assets(const asmac_view* v){
 return (v->terrain_ok?ASMAC_ART_TERRAIN:0)|(v->sprites_ok?ASMAC_ART_UNITS:0)|(v->console_ok?ASMAC_ART_CONSOLE:0)|
  (v->extras_open && v->extras.faction_colours()?ASMAC_ART_PALETTE:0)|(v->infantry_ok?ASMAC_ART_INFANTRY:0)|(v->speeder_ok?ASMAC_ART_SPEEDER:0)|
  (v->rules_ok?ASMAC_ART_RULES:0)|(v->faction_mask<<16);
}
int asmac_view_update(asmac_view* v,asmac_session* s,int reopen){
 size_t n=0;int r=asmac_view_bytes(s,&n);if(r!=ASMAC_OK)return r;
 std::vector<uint8_t> bytes(n);asmac_buffer(s,bytes.data(),n);
 const auto e=v->world.load(bytes.data(),bytes.size(),0x12345678u);
 v->world_ok=e==alpha::WorldLoadError::ok;if(!v->world_ok)return ASMAC_IO;
 if(reopen || !v->extras_open){
  // As the play window: every faction's art, a missing file leaves its bases undrawn.
  v->faction_mask=0;const alpha::PcxImage* art[8]{};
  for(unsigned f=0;f<8;++f){const auto* def=v->world.faction_definition(f);if(!def || !def->filename[0])continue;
   char name[32];unsigned len=0;bool bad=false;
   for(;len<24 && def->filename[len];++len){unsigned char c=(unsigned char)def->filename[len];if(c>='A' && c<='Z')c=(unsigned char)(c-'A'+'a');
    if(!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-'))bad=true;name[len]=(char)c;}
   if(bad || !len || len>=24)continue;memcpy(name+len,".pcx",5);
   if(load_pcx(v,name,v->faction[f],1024,768)){v->faction_mask|=1u<<f;art[f]=&v->faction[f];}}
  v->extras.open(v->world,art,reader,v);v->extras_open=true;
  v->extras.set_viewer(v->world.game().player);
 }
 return ASMAC_OK;
}
void asmac_view_set_viewer(asmac_view* v,int viewer){v->extras.set_viewer(viewer);}
int asmac_view_viewer(const asmac_view* v){return v->extras.viewer();}

/* ---- the draw() of original_map_window.cpp on a W x H frame ---- */
namespace {
struct Canvas { uint8_t* frame;int W,H,zoom,cx,cy;const asmac_view* v; };
const alpha::OriginalWorld& W_(const Canvas& c){return c.v->world;}
void pixel(const Canvas& c,int x,int y,const uint8_t* rgb){if(x<0 || y<0 || x>=c.W || y>=c.H || !rgb)return;auto* p=c.frame+((size_t)y*c.W+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
bool transparent(const uint8_t* rgb,bool sprite){
 if(!rgb)return true;
 if(sprite)return (rgb[0]==152 && rgb[1]==24 && rgb[2]==228) || (rgb[0]==100 && rgb[1]==16 && rgb[2]==156) || (rgb[0]==24 && rgb[1]==184 && rgb[2]==228);
 return rgb[0]==125 && rgb[1]==0 && rgb[2]==128;
}
void diamond(const Canvas& c,int cx,int cy,int ax,int ay,bool overlay){
 const int z=c.zoom;
 for(int dy=-16*z;dy<16*z;++dy)for(int dx=-32*z;dx<32*z;++dx){
  int u=dx+2*dy+32*z,v=2*dy-dx+32*z;if(u<0 || v<0 || u>=64*z || v>=64*z)continue;
  const auto* col=c.v->terrain.rgb((uint32_t)(ax+u*56/(64*z)),(uint32_t)(ay+v*56/(64*z)));if(!overlay || !transparent(col,false))pixel(c,cx+dx,cy+dy,col);}
}
int wrap(const Canvas& c,int x){int width=(int)W_(c).game().map.info.width;if(!W_(c).game().map.info.flat){if(x<0)x+=width;else if(x>=width)x-=width;}return x;}
const alpha::OriginalMapTile* tile(const Canvas& c,int x,int y){const auto& i=W_(c).game().map.info;if(x<0 || y<0 || x>=(int)i.width || y>=(int)i.height || ((x+y)&1))return nullptr;return W_(c).tile(((size_t)y*i.width+x)/2);}
bool position(const Canvas& c,int x,int y,int& sx,int& sy){int dx=x-c.cx,width=(int)W_(c).game().map.info.width;if(!W_(c).game().map.info.flat){if(dx>width/2)dx-=width;else if(dx< -width/2)dx+=width;}
 sx=c.W/2+dx*32*c.zoom;sy=c.H/2+(y-c.cy)*16*c.zoom;return sx>=-64 && sx<c.W+64 && sy>=-64 && sy<c.H+64;}
void sprite(const Canvas& c,int cx,int cy,int row,int column){const int z=c.zoom;for(int y=0;y<48*z;++y)for(int x=0;x<64*z;++x){const auto* col=c.v->sprites.rgb(1u+(uint32_t)column*102u+(uint32_t)(x*100/(64*z)),1u+(uint32_t)row*77u+(uint32_t)(y*76/(48*z)));if(!transparent(col,true))pixel(c,cx-32*z+x,cy-37*z+y,col);}}
void base_sprite(const Canvas& c,int cx,int cy,const alpha::OriginalBase& b){
 const auto* t=tile(c,b.x,b.y);if(!t || b.faction_id<0 || b.faction_id>=8 || !(c.v->faction_mask&(1u<<b.faction_id)))return;
 const int column=b.pop_size>=15?3:b.pop_size>=8?2:b.pop_size>=4?1:0;
 const int defense=(b.facilities_built[0]&32u)?2:(b.facilities_built[0]&16u)?1:0;
 const int row=((t->climate&0xe0u)<0x60?3:0)+defense;const auto& image=c.v->faction[(unsigned)b.faction_id];const int z=c.zoom;
 for(int y=0;y<48*z;++y)for(int x=0;x<64*z;++x){const auto* col=image.rgb(1u+(uint32_t)column*101u+(uint32_t)(x*100/(64*z)),1u+(uint32_t)row*76u+(uint32_t)(y*75/(48*z)));
  if(col && !(col[0]==255 && col[1]==0 && col[2]==255))pixel(c,cx-32*z+x,cy-36*z+y,col);}
}
bool profile(const Canvas& c,const alpha::OriginalVehicle& v,int chassis,bool ready){
 const auto* p=W_(c).prototype((size_t)v.unit_id);const auto* f=W_(c).faction_definition(v.faction_id);
 return ready && p && f && v.rotate_angle>=-1 && v.rotate_angle<=7 && W_(c).game().data_version<11 && p->chassis_id==chassis && p->weapon_id==0 && p->armor_id==0 && p->reactor_id==1 && p->ability_flags==0 && !(f->rule_flags&0x80u);
}
void line(const Canvas& c,int x0,int y0,int x1,int y1,const uint8_t* rgb){
 int dx=abs(x1-x0),dy=-abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx+dy;
 for(int guard=0;guard<4096;++guard){pixel(c,x0,y0,rgb);if(x0==x1 && y0==y1)break;int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
}
void add_item(asmac_text_item_t* out,int cap,int& n,int x,int baseline,int width,int ink,const char* text){
 if(n<cap){auto& it=out[n];it.x=x;it.baseline=baseline;it.width=width;it.ink=ink;it.heading=0;snprintf(it.text,sizeof(it.text),"%s",text);}++n;
}
}

int asmac_view_draw_map(asmac_view* v,const asmac_frame_t* f,int camera_x,int camera_y,int zoom,int selected,asmac_text_item_t* labels,int cap){
 if(!v->world_ok || !f || !f->rgb || f->capacity<(size_t)f->width*f->height*3)return -1;
 Canvas c{f->rgb,(int)f->width,(int)f->height,zoom<1?1:zoom>2?2:zoom,camera_x,camera_y,v};
 auto& world=v->world;
 for(size_t i=0;i<(size_t)c.W*c.H;++i){c.frame[3*i]=6;c.frame[3*i+1]=15;c.frame[3*i+2]=24;}
 const int cols=c.W/(64*c.zoom)+3,rows=c.H/(32*c.zoom)+3;
 if(v->terrain_ok)
 for(int y=camera_y-rows;y<=camera_y+rows;++y)for(int raw=camera_x-2*cols;raw<=camera_x+2*cols;++raw){
  int x=wrap(c,raw);const auto* t=tile(c,x,y);if(!t)continue;int sx,sy;if(!position(c,x,y,sx,sy))continue;
  const bool ocean=(t->climate&0xe0u)<0x60;int tx,ty;
  unsigned variation=((unsigned)x*7u+(unsigned)y*3u)&15u;
  if(ocean){tx=280;ty=(t->climate&0xe0u)==0x40?79:136;}
  else if((t->climate&0x18u)==0){tx=1;ty=58;}
  else {tx=1+(int)(variation%4)*57;ty=((t->climate&0x18u)>=0x10?343:115)+(int)(variation/4)*57;}
  diamond(c,sx,sy,tx,ty,false);
  if(!ocean && (t->val3&0xc0u)>0x40)diamond(c,sx,sy,58,1,true);
 }
 v->extras.draw_overlays(c.frame,c.W,c.H,world,v->terrain_ok?&v->terrain:nullptr,camera_x,camera_y,c.zoom);
 int n=0;char text[96];
 // Painter order follows map rows (native): bases occlude units behind them.
 for(int map_y=camera_y-rows;map_y<=camera_y+rows;++map_y){
  for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);if(b.y!=map_y || !v->extras.tile_visible(world,b.x,b.y))continue;int sx,sy;if(position(c,b.x,b.y,sx,sy))base_sprite(c,sx,sy,b);}
  for(size_t i=0;i<world.game().vehicle_count;++i){const auto& u=*world.vehicle(i);if(u.y!=map_y || !v->extras.tile_visible(world,u.x,u.y))continue;int sx,sy;if(!position(c,u.x,u.y,sx,sy))continue;
   int row=-1,col=0;uint32_t morale;
   if(v->rules_ok && alpha::world_effective_morale(world,*v->definitions,*v->technologies,i,true,morale)==alpha::LogicError::ok)col=(int)morale;
   switch(u.unit_id){case 7:row=0;col=0;break;case 8:row=3;break;case 9:row=4;break;case 10:row=7;col=0;break;case 15:if(world.game().data_version>=11)row=5;break;}
   const bool inf=profile(c,u,0,v->infantry_ok && v->infantry.ready()),spd=profile(c,u,1,v->speeder_ok && v->speeder.ready());
   bool drawn=false;
   if(row>=0 && v->sprites_ok){sprite(c,sx,sy,row,col);drawn=true;}
   else if((v->extras.faction_colours() || !(inf || spd)) && v->extras.draw_vehicle(c.frame,c.W,c.H,world,i,sx,sy,c.zoom))drawn=true;
   else if(inf || spd){const unsigned facing=u.rotate_angle<0?0:(unsigned)u.rotate_angle&7u;
    if(sx>=0 && sx<c.W && sy>=0 && sy<c.H)drawn=(spd?v->speeder:v->infantry).draw(c.frame,(size_t)c.W*c.H*3,c.W,c.H,sx,sy,(unsigned)c.zoom,facing);}
   if(!drawn && sx>=0 && sx<c.W && sy>=0 && sy<c.H){
    // No model for this design: the native window labels it "[CVR]"; a
    // small owner-coloured marker keeps the unit visible.
    static const uint8_t ink[8][3]={{170,170,170},{220,200,40},{60,170,60},{220,80,60},{90,120,230},{210,120,40},{170,90,200},{60,190,190}};
    const uint8_t* k=ink[(unsigned)u.faction_id&7u];for(int dy=-4;dy<=4;++dy)for(int dx=-4;dx<=4;++dx)if(dx*dx+dy*dy<=16)pixel(c,sx+dx,sy-6+dy,k);
    const auto* p=world.prototype((size_t)u.unit_id);snprintf(text,sizeof(text),"%.16s [CVR]",p && p->name[0]?p->name:"Saved design");
    add_item(labels,cap,n,sx-25,sy-16,160,0,text);}
  }
 }
 static const uint8_t white[3]={255,255,255};
 if(selected>=0 && (size_t)selected<world.game().vehicle_count){const auto& u=*world.vehicle((size_t)selected);int sx,sy;
  if(v->extras.tile_visible(world,u.x,u.y) && position(c,u.x,u.y,sx,sy)){line(c,sx,sy-14,sx+29,sy,white);line(c,sx+29,sy,sx,sy+14,white);line(c,sx,sy+14,sx-29,sy,white);line(c,sx-29,sy,sx,sy-14,white);}}
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);int sx,sy;
  if(v->extras.tile_visible(world,b.x,b.y) && position(c,b.x,b.y,sx,sy) && sx>=-40 && sx<c.W && sy>0 && sy<c.H-8){
   alpha::MapViewExtras::base_label(b,text,sizeof(text));add_item(labels,cap,n,sx-24,sy+24,240,4,text);}}
 return n;
}
int asmac_view_draw_minimap(asmac_view* v,const asmac_frame_t* f,const asmac_rect_t* r,int camera_x,int camera_y,int zoom,int view_w,int view_h){
 if(!v->world_ok || !f || !r)return 0;
 return v->extras.draw_minimap(f->rgb,f->width,f->height,{r->x,r->y,r->w,r->h},v->world,v->terrain_ok?&v->terrain:nullptr,camera_x,camera_y,zoom,(unsigned)view_w,(unsigned)view_h)?1:0;
}
int asmac_view_minimap_click(const asmac_view* v,const asmac_rect_t* r,int px,int py,int* x,int* y){
 if(!v->world_ok || !r)return 0;int a=0,b=0;const bool ok=v->extras.minimap_click(v->world,{r->x,r->y,r->w,r->h},px,py,a,b);if(x)*x=a;if(y)*y=b;return ok?1:0;
}
int asmac_view_draw_base_window(asmac_view* v,const asmac_frame_t* f,const asmac_rect_t* r,int base,asmac_text_item_t* out,int cap){
 if(!v->world_ok || !f || !r || base<0 || cap<0)return -1;
 std::vector<alpha::UiTextItem> items((size_t)(cap>0?cap:1));
 const size_t n=v->extras.draw_base_window(f->rgb,f->width,f->height,{r->x,r->y,r->w,r->h},v->world,(size_t)base,v->facilities_ok?v->facilities:nullptr,nullptr,items.data(),(size_t)cap);
 for(size_t i=0;i<n && (int)i<cap;++i){out[i].x=items[i].x;out[i].baseline=items[i].baseline;out[i].width=items[i].width;out[i].ink=items[i].ink;out[i].heading=items[i].heading;snprintf(out[i].text,sizeof(out[i].text),"%s",items[i].text);}
 return (int)n;
}
int asmac_view_draw_console(const asmac_view* v,const asmac_frame_t* f,int x0,int y0){
 if(!v->console_ok || !f)return 0;
 for(unsigned y=0;y<257;++y)for(unsigned x=0;x<800;++x){const int fx=x0+(int)x,fy=y0+(int)y;if(fx<0 || fy<0 || fx>=(int)f->width || fy>=(int)f->height)continue;
  const auto* c=v->console.rgb(x,y);if(!c || (c[0]==100 && c[1]==16 && c[2]==156) || (c[0]==125 && c[1]==0 && c[2]==128) || (c[0]==255 && c[1]==0 && c[2]==255))continue;
  auto* d=f->rgb+((size_t)fy*f->width+fx)*3;d[0]=c[0];d[1]=c[1];d[2]=c[2];}
 return 1;
}
int asmac_view_screen_tile(const asmac_view* v,int camera_x,int camera_y,int zoom,int width,int height,int px,int py,int* tx,int* ty){
 if(!v->world_ok)return 0;
 Canvas c{nullptr,width,height,zoom<1?1:zoom>2?2:zoom,camera_x,camera_y,v};
 // Nearest tile centre (native screen_tile), over the visible rows.
 int best=1<<30,bx=0,by=0;const int rows=height/(16*c.zoom)/2+3,cols=width/(32*c.zoom)/2+3;
 for(int y=camera_y-rows;y<=camera_y+rows;++y)for(int raw=camera_x-cols;raw<=camera_x+cols;++raw){const int x=wrap(c,raw);if(!tile(c,x,y))continue;int sx,sy;if(!position(c,x,y,sx,sy))continue;
  const int d=abs(px-sx)*16+abs(py-sy)*32;if(d<best){best=d;bx=x;by=y;}}
 if(best>512*c.zoom)return 0;if(tx)*tx=bx;if(ty)*ty=by;return 1;
}
int asmac_view_tile_position(const asmac_view* v,int camera_x,int camera_y,int zoom,int width,int height,int x,int y,int* sx,int* sy){
 if(!v->world_ok)return 0;Canvas c{nullptr,width,height,zoom<1?1:zoom>2?2:zoom,camera_x,camera_y,v};int a,b;const bool ok=position(c,x,y,a,b);if(sx)*sx=a;if(sy)*sy=b;return ok?1:0;
}
int asmac_view_tile_visible(const asmac_view* v,int x,int y){return v->world_ok && v->extras.tile_visible(v->world,x,y)?1:0;}
int asmac_view_messages(asmac_view* v,char* out,int cap){
 // The extras' own log lines (which art was found), newest last.
 if(!out || cap<=0)return 0;out[0]=0;std::string s;
 auto& log=v->extras.messages();for(size_t age=log.count();age-->0;){const auto* m=log.newest(age);if(m){s+=m->text;s+="\n";}}
 snprintf(out,(size_t)cap,"%s",s.c_str());return (int)log.count();
}
int asmac_view_faction_stem(const asmac_view* v,int f,char* out,int cap){
 if(!v->world_ok || f<0 || f>7 || !out || cap<=0)return 0;const auto* def=v->world.faction_definition((size_t)f);if(!def || !def->filename[0])return 0;
 int i=0;for(;i<cap-1 && i<24 && def->filename[i];++i){char ch=def->filename[i];out[i]=(ch>='A' && ch<='Z')?(char)(ch-'A'+'a'):ch;}out[i]=0;return 1;
}

/* ---- PCX images (title screens) ---- */
struct asmac_pcx { alpha::PcxImage img; };
asmac_pcx* asmac_pcx_open(const char* dir,const char* name){
 asmac_view tmp;tmp.dir=dir && dir[0]?dir:".";auto* p=new(std::nothrow) asmac_pcx();if(!p)return nullptr;
 if(!load_pcx(&tmp,name,p->img,0,0)){delete p;return nullptr;}return p;
}
void asmac_pcx_free(asmac_pcx* p){delete p;}
int asmac_pcx_size(const asmac_pcx* p,int* w,int* h){if(w)*w=(int)p->img.width();if(h)*h=(int)p->img.height();return 1;}
int asmac_pcx_rgb(const asmac_pcx* p,uint8_t* out,size_t cap){
 const size_t need=(size_t)p->img.width()*p->img.height()*3;if(cap<need)return ASMAC_SHORT_BUFFER;
 for(uint32_t y=0;y<p->img.height();++y)for(uint32_t x=0;x<p->img.width();++x){const uint8_t* c=p->img.rgb(x,y);uint8_t* d=out+((size_t)y*p->img.width()+x)*3;if(c){d[0]=c[0];d[1]=c[1];d[2]=c[2];}else d[0]=d[1]=d[2]=0;}
 return ASMAC_OK;
}
int asmac_find_file(const char* dir,const char* name,char* out,int cap){
 std::string p;if(!find_file(dir && dir[0]?dir:".",name,p))return 0;snprintf(out,(size_t)cap,"%s",p.c_str());return 1;
}

/* ---- audio catalog (audio_events.h): the Python side plays it ---- */
int asmac_audio_event_count(void){return (int)alpha::audio_event_count;}
const char* asmac_audio_event_key(int i){return i>=0 && i<(int)alpha::audio_event_count?alpha::audio_event_info((alpha::AudioEvent)i).key:"";}
int asmac_audio_event_file(const char* settings,size_t length,int i,char* out,int cap){
 if(i<0 || i>=(int)alpha::audio_event_count || !out || cap<=0)return -1;out[0]=0;
 alpha::AudioSettings s;if(settings && length && alpha::parse_audio_settings(settings,length,s)!=0)return -2;
 const char* f=alpha::audio_event_file(s,(alpha::AudioEvent)i);if(!f)return 0;snprintf(out,(size_t)cap,"%s",f);return 1;
}
int asmac_audio_levels(const char* settings,size_t length,int* out,int count){
 alpha::AudioSettings s;if(settings && length && alpha::parse_audio_settings(settings,length,s)!=0)return 0;
 const int v[7]={s.levels.master,s.levels.sfx,s.levels.voice,s.levels.music,s.levels.sfx_on,s.levels.voice_on,s.levels.music_on};
 int k=0;for(;k<count && k<7;++k)out[k]=v[k];return k;
}
const char* asmac_faction_music_set(const char* stem){const char* s=alpha::faction_music_set(stem?stem:"");return s?s:"";}
int asmac_music_set_count(void){return (int)alpha::music_set_count();}
const char* asmac_music_set(int i){const char* s=i>=0?alpha::music_set((size_t)i):nullptr;return s?s:"";}
int asmac_technology_voice_path(int id,char* out,int cap){return alpha::technology_voice_path(id,out,(size_t)cap)?1:0;}
int asmac_facility_voice_path(int id,char* out,int cap){return alpha::facility_voice_path(id,out,(size_t)cap)?1:0;}
int asmac_faction_voice_path(const char* stem,int expansion,char* out,int cap){return alpha::faction_voice_path(stem,expansion!=0,out,(size_t)cap)?1:0;}
