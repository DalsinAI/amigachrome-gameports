/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only original-world renderer. Texture atlas coordinates are asset layout
 * facts checked against GLSMAC Consts.h at0fe9ef16; no GLSMAC code is copied.
 * Flat projection is new presentation wiring, not the original slope renderer.
 */
#include "original_world.h"
#include "world_movement_price.h"
#include "pcx_image.h"
#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct IntuitionBase* IntuitionBase;struct GfxBase* GfxBase;struct Library* CyberGfxBase;
static alpha::OriginalWorld world;static alpha::PcxImage terrain,sprites,faction_images[8];
static alpha::Rules technologies;static alpha::UnitDefinitions definitions;static alpha::BasicRules basic;static alpha::WorldRules world_rules;
static const int W=928,H=480;static uint8_t* frame;static size_t selected=0;static int camera_x,camera_y,zoom=2;
static uint8_t* read_file(const char* path,size_t& size){
 FILE* f=fopen(path,"rb");if(!f)return nullptr;if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);
 if(n<=0 || n>64*1024*1024){fclose(f);return nullptr;}auto* p=(uint8_t*)malloc((size_t)n);if(!p){fclose(f);return nullptr;}
 bool ok=fread(p,1,(size_t)n,f)==(size_t)n;fclose(f);if(!ok){free(p);return nullptr;}size=(size_t)n;return p;
}
static bool load_image(const char* path,alpha::PcxImage& image){size_t n;auto* p=read_file(path,n);if(!p)return false;bool ok=image.load(p,n);free(p);return ok && image.width()==1024 && image.height()==768;}
static bool transparent(const uint8_t* rgb,bool sprite){
 if(!rgb)return true;
 if(sprite)return (rgb[0]==152 && rgb[1]==24 && rgb[2]==228) || (rgb[0]==100 && rgb[1]==16 && rgb[2]==156) || (rgb[0]==24 && rgb[1]==184 && rgb[2]==228);
 return rgb[0]==125 && rgb[1]==0 && rgb[2]==128;
}
static void pixel(int x,int y,const uint8_t* rgb){if(x<0 || y<0 || x>=W || y>=H || !rgb)return;auto* p=frame+((size_t)y*W+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
static void diamond(int cx,int cy,int atlas_x,int atlas_y,bool overlay){
 // Integer inverse of a flat isometric56x56 texture quad. Every destination
 // pixel maps into exactly one bounded source cell; no float/endian casts.
 for(int dy=-16*zoom;dy<16*zoom;++dy)for(int dx=-32*zoom;dx<32*zoom;++dx){
  int u=dx+2*dy+32*zoom,v=2*dy-dx+32*zoom;if(u<0 || v<0 || u>=64*zoom || v>=64*zoom)continue;
  const auto* c=terrain.rgb((uint32_t)(atlas_x+u*56/(64*zoom)),(uint32_t)(atlas_y+v*56/(64*zoom)));if(!overlay || !transparent(c,false))pixel(cx+dx,cy+dy,c);
 }
}
static int wrap(int x){int width=(int)world.game().map.info.width;if(!world.game().map.info.flat){if(x<0)x+=width;else if(x>=width)x-=width;}return x;}
static const alpha::OriginalMapTile* tile(int x,int y){if(x<0 || y<0 || x>=(int)world.game().map.info.width || y>=(int)world.game().map.info.height || ((x+y)&1))return nullptr;return world.tile(((size_t)y*world.game().map.info.width+x)/2);}
static bool position(int x,int y,int& sx,int& sy){int dx=x-camera_x,width=(int)world.game().map.info.width;if(!world.game().map.info.flat){if(dx>width/2)dx-=width;else if(dx< -width/2)dx+=width;}sx=W/2+dx*32*zoom;sy=H/2+(y-camera_y)*16*zoom;return sx>=-64 && sx<W+64 && sy>=-64 && sy<H+64;}
static void sprite(int cx,int cy,int row,int column){for(int y=0;y<48*zoom;++y)for(int x=0;x<64*zoom;++x){const auto* c=sprites.rgb(1u+(uint32_t)column*102u+(uint32_t)(x*100/(64*zoom)),1u+(uint32_t)row*77u+(uint32_t)(y*76/(48*zoom)));if(!transparent(c,true))pixel(cx-32*zoom+x,cy-37*zoom+y,c);}}
static void base_sprite(int cx,int cy,const alpha::OriginalBase& b){
 const auto* t=tile(b.x,b.y);if(!t || b.faction_id<0 || b.faction_id>=8)return;
 const int column=b.pop_size>=15?3:b.pop_size>=8?2:b.pop_size>=4?1:0;
 const int defense=(b.facilities_built[0]&32u)?2:(b.facilities_built[0]&16u)?1:0;
 const int row=((t->climate&0xe0u)<0x60?3:0)+defense;const auto& image=faction_images[(unsigned)b.faction_id];
 for(int y=0;y<48*zoom;++y)for(int x=0;x<64*zoom;++x){const auto* c=image.rgb(1u+(uint32_t)column*101u+(uint32_t)(x*100/(64*zoom)),1u+(uint32_t)row*76u+(uint32_t)(y*75/(48*zoom)));
  if(c && !(c[0]==255 && c[1]==0 && c[2]==255))pixel(cx-32*zoom+x,cy-36*zoom+y,c);
 }
}

static void text(struct RastPort* rp,int x,int y,const char* s){Move(rp,x,y);Text(rp,(STRPTR)s,(ULONG)strlen(s));}
static void draw(struct Window* window){
 for(size_t i=0;i<(size_t)W*H;++i){frame[3*i]=6;frame[3*i+1]=15;frame[3*i+2]=24;}
 for(int y=camera_y-17;y<=camera_y+17;++y)for(int raw=camera_x-17;raw<=camera_x+17;++raw){
  int x=wrap(raw);const auto* t=tile(x,y);if(!t)continue;int sx,sy;if(!position(x,y,sx,sy))continue;
  const bool ocean=(t->climate&0xe0u)<0x60;int tx,ty;
  unsigned variation=((unsigned)x*7u+(unsigned)y*3u)&15u;
  if(ocean){tx=280;ty=(t->climate&0xe0u)==0x40?79:136;}
  else if((t->climate&0x18u)==0){tx=1;ty=58;}
  else {tx=1+(int)(variation%4)*57;ty=((t->climate&0x18u)>=0x10?343:115)+(int)(variation/4)*57;}
  diamond(sx,sy,tx,ty,false);
  if(!ocean && (t->val3&0xc0u)>0x40)diamond(sx,sy,58,1,true);
  if(t->items&0x200000u)diamond(sx,sy,697,177,true);
  if(t->items&0x20u)diamond(sx,sy,ocean?679:451,687,true);
  if(t->items&0x80u)diamond(sx,sy,451,430,true);
  if(t->items&0x8000u)diamond(sx,sy,775,219,true);
 }
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);int sx,sy;if(position(b.x,b.y,sx,sy))base_sprite(sx,sy,b);}
 for(size_t i=0;i<world.game().vehicle_count;++i){const auto& v=*world.vehicle(i);int sx,sy;if(!position(v.x,v.y,sx,sy))continue;int row=-1,col=0;uint32_t morale;
  if(alpha::world_effective_morale(world,definitions,technologies,i,true,morale)==alpha::LogicError::ok)col=(int)morale;
  switch(v.unit_id){case 7:row=0;col=0;break;case 8:row=3;break;case 9:row=4;break;case 10:row=7;col=0;break;case 15:if(world.game().data_version>=11)row=5;break;}
  if(row>=0)sprite(sx,sy,row,col);
 }
 auto* rp=window->RPort;WritePixelArray(frame,0,0,(UWORD)(W*3),rp,12,70,W,H,RECTFMT_RGB);
 SetAPen(rp,0);RectFill(rp,window->BorderLeft,window->BorderTop,window->Width-window->BorderRight-1,64);RectFill(rp,window->BorderLeft,554,window->Width-window->BorderRight-1,window->Height-window->BorderBottom-1);
 SetAPen(rp,1);SetBPen(rp,0);SetDrMd(rp,JAM2);char line[180];
 snprintf(line,sizeof(line),"ORIGINAL TUTORIAL | Turn %ld | %lu x %lu | %lu units, %lu bases",(long)world.game().preferences.CurrentTurn,(unsigned long)world.game().map.info.width,(unsigned long)world.game().map.info.height,(unsigned long)world.game().vehicle_count,(unsigned long)world.game().base_count);text(rp,15,36,line);
 text(rp,15,55,"Original terrain/native art; read-only view. WASD scroll | TAB next unit | C centre | B base | +/- zoom | Q quit");
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);int sx,sy;if(position(b.x,b.y,sx,sy) && sx>=30 && sx<W-180 && sy>0 && sy<H-32){snprintf(line,sizeof(line),"[Base] %.24s",b.name);text(rp,12+sx-24,70+sy+24,line);}}
 for(size_t i=0;i<world.game().vehicle_count;++i){const auto& v=*world.vehicle(i);int sx,sy;if(!position(v.x,v.y,sx,sy) || sy<0 || sy>=H)continue;
  const auto& p=*world.prototype((size_t)v.unit_id);bool native=v.unit_id>=7 && v.unit_id<=10;
  if(!native && sx>=30 && sx<W-180){snprintf(line,sizeof(line),"%.16s [CVR]",p.name[0]?p.name:"Saved design");text(rp,12+sx-25,70+sy-16,line);}
  if(i==selected){Move(rp,12+sx,70+sy-14);Draw(rp,12+sx+29,70+sy);Draw(rp,12+sx,70+sy+14);Draw(rp,12+sx-29,70+sy);Draw(rp,12+sx,70+sy-14);}
 }
 if(world.game().vehicle_count){const auto& v=*world.vehicle(selected);const auto& p=*world.prototype((size_t)v.unit_id);
  snprintf(line,sizeof(line),"Selected %lu: %.31s | design %d | faction %u | original position %d,%d",(unsigned long)selected,p.name[0]?p.name:"Unnamed original design",v.unit_id,v.faction_id,v.x,v.y);text(rp,15,575,line);
  const int dx[8]={1,2,1,0,-1,-2,-1,0},dy[8]={-1,0,1,2,1,0,-1,-2};size_t used=0;used+=(size_t)snprintf(line,sizeof(line),"Adjacent prices:");
  for(unsigned d=0;d<8 && used<sizeof(line);++d){uint32_t price=0;auto error=alpha::world_movement_price(world,definitions,technologies,selected,basic.move_rate_roads,wrap(v.x+dx[d]),v.y+dy[d],false,price);used+=(size_t)snprintf(line+used,sizeof(line)-used,error==alpha::LogicError::ok?" %u:%lu":" %u:edge",d,(unsigned long)price);}text(rp,15,594,line);
 }
 text(rp,15,614,"Flat terrain preview: slopes/coast joins, human CVR models and movement actions are not restored yet.");
 printf("DRAW selected=%lu camera=%d,%d zoom=%d\n",(unsigned long)selected,camera_x,camera_y,zoom);fflush(stdout);
}
int alpha_original_view(int argc,char** argv){
 if(argc!=6 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))){puts("Usage: alpha-original-view base|expansion RULES SCENARIO texture.pcx Units.pcx");return 20;}
 size_t n;auto* p=read_file(argv[2],n);if(!p)return 20;auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;
 bool ok=alpha::read_technologies(p,n,technologies)==alpha::Error::ok && alpha::read_unit_definitions(p,n,technologies,variant,definitions)==alpha::Error::ok && alpha::read_basic_rules(p,n,technologies,variant,basic,world_rules)==alpha::Error::ok;free(p);if(!ok)return 20;
 p=read_file(argv[3],n);if(!p)return 20;ok=world.load(p,n,0x12345678u)==alpha::WorldLoadError::ok;free(p);if(!ok || (world.game().data_version<11)!=(variant==alpha::Variant::base))return 20;
 if(!load_image(argv[4],terrain) || !load_image(argv[5],sprites))return 20;
 bool needed[8]{};
 for(size_t i=0;i<world.game().base_count;++i){int f=world.base(i)->faction_id;if(f<0 || f>=8)return 20;needed[f]=true;}
 for(unsigned f=0;f<8;++f)if(needed[f]){char name[32];unsigned len=0;const auto& def=*world.faction_definition(f);
  for(;len<24 && def.filename[len];++len){unsigned char c=(unsigned char)def.filename[len];if(c>='A' && c<='Z')c=(unsigned char)(c-'A'+'a');if(!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-'))return 20;name[len]=(char)c;}
  if(!len || len>=24)return 20;
  memcpy(name+len,".pcx",5);if(!load_image(name,faction_images[f])){fprintf(stderr,"Missing faction art %s\n",name);return 20;}
 }
 frame=(uint8_t*)malloc((size_t)W*H*3);if(!frame)return 20;
 if(world.game().vehicle_count){camera_x=world.vehicle(0)->x;camera_y=world.vehicle(0)->y;}else {camera_x=(int)world.game().map.info.width/2;camera_y=(int)world.game().map.info.height/2;}
 IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);
 struct Window* window=nullptr;
 if(IntuitionBase && GfxBase && CyberGfxBase)window=OpenWindowTags(NULL,WA_Title,(ULONG)"Alpha Centauri - original tutorial data and art",WA_Width,952,WA_Height,632,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_REFRESHWINDOW,TAG_DONE);
 int rc=20;
 if(window){draw(window);bool quit=false;while(!quit){WaitPort(window->UserPort);struct IntuiMessage* m;
  while((m=(struct IntuiMessage*)GetMsg(window->UserPort))){ULONG kind=m->Class;UWORD code=m->Code;ReplyMsg((struct Message*)m);
   if(kind==IDCMP_CLOSEWINDOW)quit=true;
   else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(window);draw(window);EndRefresh(window,TRUE);}
   else if(kind==IDCMP_VANILLAKEY){switch(code){case 'q':case 'Q':quit=true;break;case 'w':camera_y-=2;break;case 's':camera_y+=2;break;case 'a':camera_x=wrap(camera_x-2);break;case 'd':camera_x=wrap(camera_x+2);break;case 'b':if(world.game().base_count){camera_x=world.base(0)->x;camera_y=world.base(0)->y;}break;case '+':case '=':zoom=2;break;case '-':zoom=1;break;case 9:if(world.game().vehicle_count)selected=(selected+1)%world.game().vehicle_count;break;case 'c':if(world.game().vehicle_count){camera_x=world.vehicle(selected)->x;camera_y=world.vehicle(selected)->y;}break;}
    if(camera_x<0)camera_x=0;
    if(camera_x>=(int)world.game().map.info.width)camera_x=(int)world.game().map.info.width-1;
    if(camera_y<0)camera_y=0;
    if(camera_y>=(int)world.game().map.info.height)camera_y=(int)world.game().map.info.height-1;
    draw(window);}
  }
 }CloseWindow(window);rc=0;}
 if(CyberGfxBase)CloseLibrary(CyberGfxBase);
 if(GfxBase)CloseLibrary((struct Library*)GfxBase);
 if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);
 free(frame);printf("ORIGINAL VIEW RC %d\n",rc);return rc;
}

#ifndef ALPHA_EMBEDDED_VIEW
int main(int argc,char** argv){return alpha_original_view(argc,argv);}
#endif
