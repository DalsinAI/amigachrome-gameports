/* SPDX-License-Identifier: GPL-3.0-or-later
 * Read-only original-world renderer. Texture atlas coordinates are asset layout
 * facts checked against GLSMAC Consts.h at0fe9ef16; no GLSMAC code is copied.
 * Flat projection is new presentation wiring, not the original slope renderer.
 */
#include "original_world.h"
#include "world_movement_price.h"
#include "pcx_image.h"
#include "original_ui_text.h"
#include "technology_browser.h"
#include "cvr_infantry.h"
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
static alpha::CvrInfantry infantry;static bool infantry_available=false;
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
static bool infantry_profile(const alpha::OriginalVehicle& v){
 const auto* p=world.prototype((size_t)v.unit_id);const auto* f=world.faction_definition(v.faction_id);
 return infantry_available && infantry.ready() && p && f && v.rotate_angle>=-1 && v.rotate_angle<=7 && world.game().data_version<11 && p->chassis_id==0 && p->weapon_id==0 && p->armor_id==0 && p->reactor_id==1 && p->ability_flags==0 && !(f->rule_flags&0x80u);
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

static const int HUD_H=257,HUD_X=64,HUD_Y=554;
static alpha::PcxImage console_image;static alpha::OriginalUiText ui_text;static uint8_t* hud_frame;
static size_t selected_base=0;static bool base_open=false;
static void hud_text(struct RastPort* rp,int x,int y,const char* s,int width=220,bool heading=false,alpha::UiInk ink=alpha::UiInk::body){ui_text.draw(rp,12+HUD_X+x,HUD_Y+y,s,width,heading,ink);}
static void production_name(const alpha::OriginalBase& base,char* out,size_t bytes){
 if(base.queue_size<0 || base.queue_size>9){snprintf(out,bytes,"Production details unavailable");return;}
 int id=base.queue_items[0];if(id>=0 && id<512){const auto* p=world.prototype((size_t)id);snprintf(out,bytes,"%.31s",p && p->name[0]?p->name:"Original unit design");}
 else snprintf(out,bytes,"Facility details not restored");
}
static void draw_hud(struct Window* window){
 for(int y=0;y<HUD_H;++y)for(int x=0;x<W;++x){auto* dst=hud_frame+((size_t)y*W+x)*3;dst[0]=7;dst[1]=19;dst[2]=22;
  if(x>=HUD_X && x<HUD_X+800){const auto* c=console_image.rgb((unsigned)(x-HUD_X),(unsigned)y);if(c && !(c[0]==100 && c[1]==16 && c[2]==156) && !(c[0]==125 && c[1]==0 && c[2]==128) && !(c[0]==255 && c[1]==0 && c[2]==255)){dst[0]=c[0];dst[1]=c[1];dst[2]=c[2];}}
 }
 // Navigation thumbnail of the actual saved world, coloured from loaded terrain.
 const auto& map=world.game().map.info;
 for(int py=0;py<174;++py)for(int px=0;px<224;++px){int y=(int)((uint32_t)py*map.height/174),x=(int)((uint32_t)px*map.width/224);if((x+y)&1)x=x+1<(int)map.width?x+1:x-1;const auto*t=tile(x,y);if(!t)continue;
  const auto*c=terrain.rgb((t->climate&0xe0)<0x60?308u:29u,(t->climate&0xe0)<0x60?105u:143u);auto*dst=hud_frame+((size_t)(52+py)*W+HUD_X+563+px)*3;if(c)for(int k=0;k<3;++k)dst[k]=c[k];
 }
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);int x=HUD_X+563+(int)((uint32_t)b.x*224/map.width),y=52+(int)((uint32_t)b.y*174/map.height);for(int dy=0;dy<3;++dy)for(int dx=0;dx<3;++dx){int px=x+dx,py=y+dy;if(px>=0 && px<W && py>=0 && py<HUD_H){auto*p=hud_frame+((size_t)py*W+px)*3;p[0]=230;p[1]=212;p[2]=144;}}}
 WritePixelArray(hud_frame,0,0,W*3,window->RPort,12,HUD_Y,W,HUD_H,RECTFMT_RGB);auto*rp=window->RPort;char line[180];

 if(world.game().vehicle_count){const auto&v=*world.vehicle(selected);const auto&p=*world.prototype((size_t)v.unit_id);
  hud_text(rp,10,86,p.name[0]?p.name:"Original unit",116,false,alpha::UiInk::value);
  snprintf(line,sizeof(line),"Position %d, %d",v.x,v.y);hud_text(rp,10,105,line,116);
  const auto& owner=*world.faction_definition(v.faction_id);snprintf(line,sizeof(line),"%.30s",owner.formal_name_faction[0]?owner.formal_name_faction:"Native life");hud_text(rp,10,123,line,116);
  snprintf(line,sizeof(line),"Damage %u",v.damage_taken);hud_text(rp,10,141,line,116);
  hud_text(rp,10,215,"TAB: select unit",116);hud_text(rp,10,234,"C: centre unit",116);
  const auto* t=tile(v.x,v.y);
  if(t){
   hud_text(rp,146,88,"TERRAIN",98,false,alpha::UiInk::label);
   hud_text(rp,146,109,(t->climate&0xe0u)<0x60?"Ocean":"Land",98,false,alpha::UiInk::value);
   if((t->climate&0xe0u)>=0x60){hud_text(rp,146,128,(t->val3&0xc0u)>=0x80?"Rocky":(t->val3&0xc0u)==0x40?"Rolling":"Flat",98);}
   if(t->items&0x20u)hud_text(rp,146,147,"Fungus",98);
   if(t->items&0x80u)hud_text(rp,146,166,"Forest",98);
   hud_text(rp,146,215,"Saved tile",98);hud_text(rp,146,234,"Read only",98);
  }
 }
 if(world.game().base_count){const auto&b=*world.base(selected_base);hud_text(rp,274,89,b.name,260,true,alpha::UiInk::title);
  snprintf(line,sizeof(line),"Population %d",b.pop_size);hud_text(rp,274,113,line);
  snprintf(line,sizeof(line),"Nutrients %ld   Minerals %ld",(long)b.nutrients_accumulated,(long)b.minerals_accumulated);hud_text(rp,274,137,line,260);
  snprintf(line,sizeof(line),"Surplus  N %+ld   M %+ld   E %+ld",(long)b.nutrient_surplus,(long)b.mineral_surplus,(long)b.energy_surplus);hud_text(rp,274,161,line,260);
  production_name(b,line,sizeof(line));hud_text(rp,274,212,"PRODUCTION (SAVED TURN)",260,false,alpha::UiInk::label);hud_text(rp,274,236,line,260,false,alpha::UiInk::value);

 }
 hud_text(rp,565,245,"Click map to navigate",210);
 ui_text.draw(rp,20,840,"Saved turn | B: next base | I: details | ESC: close | Q: quit | Orders / End Turn unavailable",910);
}
static void draw_base_details(struct Window* window){
 if(!base_open || !world.game().base_count)return;
 const auto&b=*world.base(selected_base);auto*rp=window->RPort;FillPixelArray(rp,104,120,744,398,0x00071316u);char line[160];
 ui_text.draw(rp,128,162,b.name,660,true,alpha::UiInk::title);ui_text.draw(rp,128,190,"Original saved-turn snapshot - read only",660);
 snprintf(line,sizeof(line),"Population %d    Specialists %ld",b.pop_size,(long)b.specialist_total);ui_text.draw(rp,128,228,line,660);
 snprintf(line,sizeof(line),"Nutrients: stored %ld   intake %ld   surplus %+ld",(long)b.nutrients_accumulated,(long)b.nutrient_intake,(long)b.nutrient_surplus);ui_text.draw(rp,128,260,line,660);
 snprintf(line,sizeof(line),"Minerals: stored %ld   intake %ld   surplus %+ld",(long)b.minerals_accumulated,(long)b.mineral_intake,(long)b.mineral_surplus);ui_text.draw(rp,128,292,line,660);
 snprintf(line,sizeof(line),"Energy: intake %ld   surplus %+ld",(long)b.energy_intake,(long)b.energy_surplus);ui_text.draw(rp,128,324,line,660);
 snprintf(line,sizeof(line),"Economy %ld    Labs %ld    Psych %ld",(long)b.economy_total,(long)b.labs_total,(long)b.psych_total);ui_text.draw(rp,128,356,line,660);
 production_name(b,line,sizeof(line));ui_text.draw(rp,128,398,"Production recorded in save:",660);ui_text.draw(rp,128,426,line,660,true);
 ui_text.draw(rp,128,482,"B selects the next base. ESC returns to the map.",660);
 printf("BASE SNAPSHOT index=%lu pop=%d nutrients=%ld minerals=%ld intake=%ld,%ld,%ld surplus=%ld,%ld,%ld\n",(unsigned long)selected_base,b.pop_size,(long)b.nutrients_accumulated,(long)b.minerals_accumulated,(long)b.nutrient_intake,(long)b.mineral_intake,(long)b.energy_intake,(long)b.nutrient_surplus,(long)b.mineral_surplus,(long)b.energy_surplus);fflush(stdout);
}

static void text(struct RastPort* rp,int x,int y,const char* s){if(x>=0 && x<W)ui_text.draw(rp,x,y,s,W-x);}
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
 // Painter order follows map rows, not record IDs: foreground bases must
 // occlude units behind them. Terrain remains the explicitly flat renderer.
 for(int map_y=camera_y-17;map_y<=camera_y+17;++map_y){
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);if(b.y!=map_y)continue;int sx,sy;if(position(b.x,b.y,sx,sy))base_sprite(sx,sy,b);}
 for(size_t i=0;i<world.game().vehicle_count;++i){const auto& v=*world.vehicle(i);if(v.y!=map_y)continue;int sx,sy;if(!position(v.x,v.y,sx,sy))continue;int row=-1,col=0;uint32_t morale;
  if(alpha::world_effective_morale(world,definitions,technologies,i,true,morale)==alpha::LogicError::ok)col=(int)morale;
  switch(v.unit_id){case 7:row=0;col=0;break;case 8:row=3;break;case 9:row=4;break;case 10:row=7;col=0;break;case 15:if(world.game().data_version>=11)row=5;break;}
  if(row>=0)sprite(sx,sy,row,col);
  else if(infantry_profile(v)){
   const unsigned facing=v.rotate_angle<0?0:(unsigned)v.rotate_angle&7u;
   if(sx>=0 && sx<W && sy>=0 && sy<H){bool drawn=infantry.draw(frame,(size_t)W*H*3,W,H,sx,sy,(unsigned)zoom,facing);printf("INFANTRY DRAW unit=%lu facing=%u ok=%d\n",(unsigned long)i,facing,drawn);}
  }
 }
 }
 auto* rp=window->RPort;WritePixelArray(frame,0,0,(UWORD)(W*3),rp,12,70,W,H,RECTFMT_RGB);
 FillPixelArray(rp,window->BorderLeft,window->BorderTop,window->Width-window->BorderRight-window->BorderLeft,64-window->BorderTop,0x00071316u);FillPixelArray(rp,window->BorderLeft,554,window->Width-window->BorderRight-window->BorderLeft,window->Height-window->BorderBottom-554,0x00071316u);
 SetAPen(rp,1);SetBPen(rp,0);SetDrMd(rp,JAM2);char line[180];
 snprintf(line,sizeof(line),"ORIGINAL TUTORIAL | Turn %ld | %lu x %lu | %lu units, %lu bases",(long)world.game().preferences.CurrentTurn,(unsigned long)world.game().map.info.width,(unsigned long)world.game().map.info.height,(unsigned long)world.game().vehicle_count,(unsigned long)world.game().base_count);text(rp,15,42,line);
 text(rp,15,62,"Read-only original world | Basic infantry CVR profile; other models pending | WASD scroll | TAB select | +/- zoom | Q quit");
 for(size_t i=0;i<world.game().base_count;++i){const auto& b=*world.base(i);int sx,sy;if(position(b.x,b.y,sx,sy) && sx>=30 && sx<W-180 && sy>0 && sy<H-32){snprintf(line,sizeof(line),"[Base] %.24s",b.name);ui_text.draw(rp,12+sx-24,70+sy+24,line,W-(12+sx-24),false,alpha::UiInk::map_label);}}
 for(size_t i=0;i<world.game().vehicle_count;++i){const auto& v=*world.vehicle(i);int sx,sy;if(!position(v.x,v.y,sx,sy) || sy<0 || sy>=H)continue;
  const auto& p=*world.prototype((size_t)v.unit_id);bool native=(v.unit_id>=7 && v.unit_id<=10) || infantry_profile(v);
  if(!native && sx>=30 && sx<W-180){snprintf(line,sizeof(line),"%.16s [CVR]",p.name[0]?p.name:"Saved design");text(rp,12+sx-25,70+sy-16,line);}
  if(i==selected){Move(rp,12+sx,70+sy-14);Draw(rp,12+sx+29,70+sy);Draw(rp,12+sx,70+sy+14);Draw(rp,12+sx-29,70+sy);Draw(rp,12+sx,70+sy-14);}
 }
 draw_hud(window);draw_base_details(window);
 printf("DRAW selected=%lu camera=%d,%d zoom=%d\n",(unsigned long)selected,camera_x,camera_y,zoom);fflush(stdout);
}
int alpha_original_view(int argc,char** argv){
 if(argc!=6 || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))){puts("Usage: alpha-original-view base|expansion RULES SCENARIO texture.pcx Units.pcx");return 20;}
 size_t n;auto* p=read_file(argv[2],n);if(!p)return 20;auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;
 bool ok=alpha::read_technologies(p,n,technologies)==alpha::Error::ok && alpha::read_unit_definitions(p,n,technologies,variant,definitions)==alpha::Error::ok && alpha::read_basic_rules(p,n,technologies,variant,basic,world_rules)==alpha::Error::ok;free(p);if(!ok)return 20;
 p=read_file(argv[3],n);if(!p)return 20;ok=world.load(p,n,0x12345678u)==alpha::WorldLoadError::ok;free(p);if(!ok || (world.game().data_version<11)!=(variant==alpha::Variant::base))return 20;
 if(!load_image(argv[4],terrain) || !load_image(argv[5],sprites))return 20;
 p=read_file("console.pcx",n);if(!p)return 20;ok=console_image.load(p,n);free(p);if(!ok || console_image.width()!=800 || console_image.height()!=257)return 20;
 selected=selected_base=0;base_open=false;
 bool needed[8]{};
 for(size_t i=0;i<world.game().base_count;++i){int f=world.base(i)->faction_id;if(f<0 || f>=8)return 20;needed[f]=true;}
 for(unsigned f=0;f<8;++f)if(needed[f]){char name[32];unsigned len=0;const auto& def=*world.faction_definition(f);
  for(;len<24 && def.filename[len];++len){unsigned char c=(unsigned char)def.filename[len];if(c>='A' && c<='Z')c=(unsigned char)(c-'A'+'a');if(!((c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='-'))return 20;name[len]=(char)c;}
  if(!len || len>=24)return 20;
  memcpy(name+len,".pcx",5);if(!load_image(name,faction_images[f])){fprintf(stderr,"Missing faction art %s\n",name);return 20;}
 }
 // Optional original models: missing/unsupported assets keep the explicit CVR
 // label instead of silently substituting a different chassis.
 size_t body_size=0,reactor_size=0;auto* body_data=read_file("VI.cvr",body_size);auto* reactor_data=read_file("Viptr00.cvr",reactor_size);
 infantry_available=body_data && reactor_data && infantry.load(body_data,body_size,reactor_data,reactor_size);free(body_data);free(reactor_data);
 printf("INFANTRY PROFILE ready=%d base-human-basic-static; tile-foot anchor; no original viewport/shadows\n",infantry_available);fflush(stdout);
 if(!ui_text.open()){puts("Original UI fonts are required.");return 20;}
 frame=(uint8_t*)malloc((size_t)W*H*3);hud_frame=(uint8_t*)malloc((size_t)W*HUD_H*3);if(!frame || !hud_frame){free(frame);free(hud_frame);ui_text.close();return 20;}
 if(world.game().vehicle_count){camera_x=world.vehicle(0)->x;camera_y=world.vehicle(0)->y;}else {camera_x=(int)world.game().map.info.width/2;camera_y=(int)world.game().map.info.height/2;}
 IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);
 struct Window* window=nullptr;
 if(IntuitionBase && GfxBase && CyberGfxBase)window=OpenWindowTags(NULL,WA_Title,(ULONG)"Alpha Centauri - original tutorial data and art",WA_Width,952,WA_Height,864,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
 int rc=20;
 if(window){draw(window);bool quit=false;while(!quit){WaitPort(window->UserPort);struct IntuiMessage* m;
  while((m=(struct IntuiMessage*)GetMsg(window->UserPort))){ULONG kind=m->Class;UWORD code=m->Code;WORD mouse_x=m->MouseX,mouse_y=m->MouseY;ReplyMsg((struct Message*)m);
   if(kind==IDCMP_CLOSEWINDOW)quit=true;
   else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(window);draw(window);EndRefresh(window,TRUE);}
   else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
    if(mouse_x>=12+HUD_X+563 && mouse_x<12+HUD_X+787 && mouse_y>=HUD_Y+52 && mouse_y<HUD_Y+226){camera_x=(int)((uint32_t)(mouse_x-12-HUD_X-563)*world.game().map.info.width/224);camera_y=(int)((uint32_t)(mouse_y-HUD_Y-52)*world.game().map.info.height/174);}
    else for(size_t i=0;i<world.game().base_count;++i){int x,y;const auto&b=*world.base(i);if(position(b.x,b.y,x,y) && abs(mouse_x-12-x)<24*zoom && abs(mouse_y-70-y)<24*zoom){selected_base=i;base_open=true;break;}}
    draw(window);
   }
   else if(kind==IDCMP_VANILLAKEY){switch(code){case 't':case 'T':{int rc=alpha_technology_browser(argv[2]);printf("WORLD TECHNOLOGY RC %d\n",rc);fflush(stdout);ActivateWindow(window);}break;case 'q':case 'Q':quit=true;break;case 'w':camera_y-=2;break;case 's':camera_y+=2;break;case 'a':camera_x=wrap(camera_x-2);break;case 'd':camera_x=wrap(camera_x+2);break;case 'b':if(world.game().base_count){selected_base=(selected_base+1)%world.game().base_count;camera_x=world.base(selected_base)->x;camera_y=world.base(selected_base)->y;}break;case 'i':if(world.game().base_count)base_open=!base_open;break;case 27:base_open=false;break;case '+':case '=':zoom=2;break;case '-':zoom=1;break;case 9:if(world.game().vehicle_count)selected=(selected+1)%world.game().vehicle_count;break;case 'c':if(world.game().vehicle_count){camera_x=world.vehicle(selected)->x;camera_y=world.vehicle(selected)->y;}break;}
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
 free(frame);free(hud_frame);ui_text.close();printf("ORIGINAL VIEW RC %d\n",rc);return rc;
}

#ifndef ALPHA_EMBEDDED_VIEW
int main(int argc,char** argv){return alpha_original_view(argc,argv);}
#endif
