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
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
#include "local_player.h"
#include "world_base_accounting.h"
#include "world_base_budget.h"
#include <proto/dos.h>
#include <dos/dos.h>
#endif
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
#include <errno.h>
struct IntuitionBase* IntuitionBase;struct GfxBase* GfxBase;struct Library* CyberGfxBase;
static alpha::CvrInfantry infantry;static bool infantry_available=false;
static alpha::OriginalWorld preview_world;
static const alpha::OriginalWorld* visible_world=&preview_world;
static const alpha::OriginalWorld& world(){return *visible_world;}
static bool local_enabled=false;
static char action_status[180]="";
static char source_label[81]="Original world";
static void set_source_label(const char* path){
 const char* name=path;
 for(const char* p=path;*p;++p)if(*p=='/' || *p==':' || *p=='\\')name=p+1;
 if(!*name){snprintf(source_label,sizeof(source_label),"Original world");return;}
 size_t length=0;
 while(name[length] && length+1<sizeof(source_label)){
  const unsigned char c=(unsigned char)name[length];
  source_label[length]=(c<32 || c==127)?'?':(char)c;++length;
 }
 source_label[length]=0;
}
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
static alpha::LocalPlayer player;
static uint8_t* player_rules;static size_t player_rules_length;
static char last_checkpoint[256];static char discard_pending=0;
static alpha::FacilityDefinitions facility_labels;
static alpha::ResourceInfo forecast_resources;
static alpha::WorldAccountingResult forecast;
static alpha::CitizenDefinitions forecast_citizens;
static alpha::BaseBudgetResult forecast_budget;
static bool forecast_citizens_ready=false,forecast_budget_ready=false;
static bool forecast_resources_ready=false,forecast_open=false;
static size_t forecast_base=0;
struct PlayerViewScope {
 ~PlayerViewScope(){forecast_open=false;forecast_resources_ready=false;player.close();visible_world=&preview_world;local_enabled=false;free(player_rules);player_rules=nullptr;player_rules_length=0;}
};
#endif
static alpha::PcxImage terrain,sprites,faction_images[8];
static alpha::Rules technologies;static alpha::UnitDefinitions definitions;static alpha::BasicRules basic;static alpha::WorldRules world_rules;
static const int W=928,H=480;static uint8_t* frame;static size_t selected=0;static int camera_x,camera_y,zoom=2;
static uint8_t* read_file(const char* path,size_t& size){
 FILE* f=fopen(path,"rb");if(!f){fprintf(stderr,"Cannot open world asset %s: %s\n",path,strerror(errno));return nullptr;}if(fseek(f,0,SEEK_END)){fclose(f);return nullptr;}long n=ftell(f);rewind(f);
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
 const auto* p=world().prototype((size_t)v.unit_id);const auto* f=world().faction_definition(v.faction_id);
 return infantry_available && infantry.ready() && p && f && v.rotate_angle>=-1 && v.rotate_angle<=7 && world().game().data_version<11 && p->chassis_id==0 && p->weapon_id==0 && p->armor_id==0 && p->reactor_id==1 && p->ability_flags==0 && !(f->rule_flags&0x80u);
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
static int wrap(int x){int width=(int)world().game().map.info.width;if(!world().game().map.info.flat){if(x<0)x+=width;else if(x>=width)x-=width;}return x;}
static const alpha::OriginalMapTile* tile(int x,int y){if(x<0 || y<0 || x>=(int)world().game().map.info.width || y>=(int)world().game().map.info.height || ((x+y)&1))return nullptr;return world().tile(((size_t)y*world().game().map.info.width+x)/2);}
static bool position(int x,int y,int& sx,int& sy){int dx=x-camera_x,width=(int)world().game().map.info.width;if(!world().game().map.info.flat){if(dx>width/2)dx-=width;else if(dx< -width/2)dx+=width;}sx=W/2+dx*32*zoom;sy=H/2+(y-camera_y)*16*zoom;return sx>=-64 && sx<W+64 && sy>=-64 && sy<H+64;}
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
static void select_local_actors(){
 selected=selected_base=0;
 for(size_t i=0;i<world().game().vehicle_count;++i)if(world().vehicle(i)->faction_id==world().game().player){selected=i;break;}
 // Prefer an idle ordinary surface design to an ordered garrison. Selection
 // is advisory: the live command still checks cargo, terrain and all gates.
 for(size_t i=0;i<world().game().vehicle_count;++i){const auto& v=*world().vehicle(i);const auto* p=v.unit_id>=0?world().prototype((size_t)v.unit_id):nullptr;
  if(v.faction_id!=world().game().player || v.order || v.waypoint_count || (v.state&0x0b004201u) ||
     !p || p->ability_flags || p->chassis_id>=9 || p->weapon_id>=definitions.weapon_count || definitions.weapons[p->weapon_id].offense<0)continue;
  const int triad=definitions.chassis[p->chassis_id].triad;
  if((triad==0 && p->plan>=0 && p->plan<=3) || (triad==1 && ((p->plan>=0 && p->plan<=3) || p->plan==7))){selected=i;break;}
 }
 for(size_t i=0;i<world().game().base_count;++i)if(world().base(i)->faction_id==world().game().player){selected_base=i;break;}
}
static void hud_text(struct RastPort* rp,int x,int y,const char* s,int width=220,bool heading=false,alpha::UiInk ink=alpha::UiInk::body){ui_text.draw(rp,12+HUD_X+x,HUD_Y+y,s,width,heading,ink);}
static void production_name(const alpha::OriginalBase& base,char* out,size_t bytes){
 if(base.queue_size<0 || base.queue_size>9){snprintf(out,bytes,"Production details unavailable");return;}
 int id=base.queue_items[0];if(id>=0 && id<512){const auto* p=world().prototype((size_t)id);snprintf(out,bytes,"%.31s",p && p->name[0]?p->name:"Original unit design");}
 else {
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
  if(local_enabled && id<0 && id>-(int)facility_labels.slot_count){snprintf(out,bytes,"%.80s",facility_labels.slots[-id].name);return;}
#endif
  snprintf(out,bytes,"Facility details not restored");
 }
}
static void draw_hud(struct Window* window){
 for(int y=0;y<HUD_H;++y)for(int x=0;x<W;++x){auto* dst=hud_frame+((size_t)y*W+x)*3;dst[0]=7;dst[1]=19;dst[2]=22;
  if(x>=HUD_X && x<HUD_X+800){const auto* c=console_image.rgb((unsigned)(x-HUD_X),(unsigned)y);if(c && !(c[0]==100 && c[1]==16 && c[2]==156) && !(c[0]==125 && c[1]==0 && c[2]==128) && !(c[0]==255 && c[1]==0 && c[2]==255)){dst[0]=c[0];dst[1]=c[1];dst[2]=c[2];}}
 }
 // Navigation thumbnail of the actual saved world, coloured from loaded terrain.
 const auto& map=world().game().map.info;
 for(int py=0;py<174;++py)for(int px=0;px<224;++px){int y=(int)((uint32_t)py*map.height/174),x=(int)((uint32_t)px*map.width/224);if((x+y)&1)x=x+1<(int)map.width?x+1:x-1;const auto*t=tile(x,y);if(!t)continue;
  const auto*c=terrain.rgb((t->climate&0xe0)<0x60?308u:29u,(t->climate&0xe0)<0x60?105u:143u);auto*dst=hud_frame+((size_t)(52+py)*W+HUD_X+563+px)*3;if(c)for(int k=0;k<3;++k)dst[k]=c[k];
 }
 for(size_t i=0;i<world().game().base_count;++i){const auto& b=*world().base(i);int x=HUD_X+563+(int)((uint32_t)b.x*224/map.width),y=52+(int)((uint32_t)b.y*174/map.height);for(int dy=0;dy<3;++dy)for(int dx=0;dx<3;++dx){int px=x+dx,py=y+dy;if(px>=0 && px<W && py>=0 && py<HUD_H){auto*p=hud_frame+((size_t)py*W+px)*3;p[0]=230;p[1]=212;p[2]=144;}}}
 WritePixelArray(hud_frame,0,0,W*3,window->RPort,12,HUD_Y,W,HUD_H,RECTFMT_RGB);auto*rp=window->RPort;char line[180];

 if(world().game().vehicle_count){const auto&v=*world().vehicle(selected);const auto&p=*world().prototype((size_t)v.unit_id);
  hud_text(rp,10,86,p.name[0]?p.name:"Original unit",116,false,alpha::UiInk::value);
  snprintf(line,sizeof(line),"Position %d, %d",v.x,v.y);hud_text(rp,10,105,line,116);
  const auto& owner=*world().faction_definition(v.faction_id);snprintf(line,sizeof(line),"%.30s",owner.formal_name_faction[0]?owner.formal_name_faction:"Native life");hud_text(rp,10,123,line,116);
  snprintf(line,sizeof(line),"Damage %u",v.damage_taken);hud_text(rp,10,141,line,116);
  hud_text(rp,10,215,"TAB: select unit",116);hud_text(rp,10,234,"C: centre unit",116);
  const auto* t=tile(v.x,v.y);
  if(t){
   hud_text(rp,146,88,"TERRAIN",98,false,alpha::UiInk::label);
   hud_text(rp,146,109,(t->climate&0xe0u)<0x60?"Ocean":"Land",98,false,alpha::UiInk::value);
   if((t->climate&0xe0u)>=0x60){hud_text(rp,146,128,(t->val3&0xc0u)>=0x80?"Rocky":(t->val3&0xc0u)==0x40?"Rolling":"Flat",98);}
   if(t->items&0x20u)hud_text(rp,146,147,"Fungus",98);
   if(t->items&0x80u)hud_text(rp,146,166,"River",98);
   if(t->items&0x200000u)hud_text(rp,146,184,"Forest",98);
   hud_text(rp,146,215,"Saved tile",98);hud_text(rp,146,234,local_enabled?"Local actions":"Read only",98);
  }
 }
 if(world().game().base_count){const auto&b=*world().base(selected_base);hud_text(rp,274,89,b.name,260,true,alpha::UiInk::title);
  snprintf(line,sizeof(line),"Population %d",b.pop_size);hud_text(rp,274,113,line);
  snprintf(line,sizeof(line),"Nutrients %ld   Minerals %ld",(long)b.nutrients_accumulated,(long)b.minerals_accumulated);hud_text(rp,274,137,line,260);
  snprintf(line,sizeof(line),"Surplus  N %+ld   M %+ld   E %+ld",(long)b.nutrient_surplus,(long)b.mineral_surplus,(long)b.energy_surplus);hud_text(rp,274,161,line,260);
  production_name(b,line,sizeof(line));hud_text(rp,274,212,"PRODUCTION (SAVED TURN)",260,false,alpha::UiInk::label);hud_text(rp,274,236,line,260,false,alpha::UiInk::value);

 }
 hud_text(rp,565,245,"Click map to navigate",210);
 ui_text.draw(rp,20,840,local_enabled?action_status:"Saved turn | B: next base | I: details | ESC: close | Q: quit | Orders / End Turn unavailable",910);
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 if(local_enabled && world().game().vehicle_count){const auto& v=*world().vehicle(selected);uint32_t speed=0;
  if(alpha::world_vehicle_speed(world(),definitions,technologies,selected,basic.move_rate_roads,false,speed)==alpha::LogicError::ok){char budget[96];snprintf(budget,sizeof(budget),"Moves %lu/%lu%s",(unsigned long)(speed>v.moves_spent?speed-v.moves_spent:0),(unsigned long)speed,player.dirty()?" *":"");hud_text(rp,10,160,budget,116);}
 }
#endif
}
static void draw_base_details(struct Window* window){
 if(!base_open || !world().game().base_count)return;
 const auto&b=*world().base(selected_base);auto*rp=window->RPort;FillPixelArray(rp,104,120,744,398,0x00071316u);char line[160];
 ui_text.draw(rp,128,162,b.name,660,true,alpha::UiInk::title);ui_text.draw(rp,128,190,local_enabled?"Local base - P chooses a facility; ENTER confirms":"Original saved-turn snapshot - read only",660);
 snprintf(line,sizeof(line),"Population %d    Specialists %ld",b.pop_size,(long)b.specialist_total);ui_text.draw(rp,128,228,line,660);
 snprintf(line,sizeof(line),"Nutrients: stored %ld   intake %ld   surplus %+ld",(long)b.nutrients_accumulated,(long)b.nutrient_intake,(long)b.nutrient_surplus);ui_text.draw(rp,128,260,line,660);
 snprintf(line,sizeof(line),"Minerals: stored %ld   intake %ld   surplus %+ld",(long)b.minerals_accumulated,(long)b.mineral_intake,(long)b.mineral_surplus);ui_text.draw(rp,128,292,line,660);
 snprintf(line,sizeof(line),"Energy: intake %ld   surplus %+ld",(long)b.energy_intake,(long)b.energy_surplus);ui_text.draw(rp,128,324,line,660);
 snprintf(line,sizeof(line),"Economy %ld    Labs %ld    Psych %ld",(long)b.economy_total,(long)b.labs_total,(long)b.psych_total);ui_text.draw(rp,128,356,line,660);
 production_name(b,line,sizeof(line));ui_text.draw(rp,128,398,"Production recorded in save:",660);ui_text.draw(rp,128,426,line,660,true);
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 const auto* proposed=local_enabled?player.proposal():nullptr;
 if(proposed && proposed->base==(int32_t)selected_base){
  snprintf(line,sizeof(line),"Build %.56s",facility_labels.slots[proposed->facility].name);ui_text.draw(rp,128,454,line,660);
  snprintf(line,sizeof(line),"Minerals %ld -> %ld (switch loss %ld). ENTER confirms; ESC cancels.",(long)proposed->mineral_before,(long)proposed->mineral_after,(long)proposed->mineral_loss);ui_text.draw(rp,128,482,line,660);
 }else
#endif
 ui_text.draw(rp,128,482,"B selects the next base. ESC returns to the map.",660);
 printf("BASE SNAPSHOT index=%lu pop=%d nutrients=%ld minerals=%ld intake=%ld,%ld,%ld surplus=%ld,%ld,%ld\n",(unsigned long)selected_base,b.pop_size,(long)b.nutrients_accumulated,(long)b.minerals_accumulated,(long)b.nutrient_intake,(long)b.mineral_intake,(long)b.energy_intake,(long)b.nutrient_surplus,(long)b.mineral_surplus,(long)b.energy_surplus);fflush(stdout);
}

#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
static void open_forecast(){
 forecast_open=false;forecast_budget_ready=false;
 if(player.proposal()){snprintf(action_status,sizeof(action_status),"Confirm or cancel the facility order before opening the forecast.");return;}
 const auto* current=player.world();
 const auto* base=current?current->base(selected_base):nullptr;
 if(!current || !base || base->faction_id!=current->game().player){snprintf(action_status,sizeof(action_status),"Choose one of your bases to see its forecast.");return;}
 if(!forecast_resources_ready){snprintf(action_status,sizeof(action_status),"The original resource rules could not be read.");return;}
 const size_t count=current->game().vehicle_count;
 if(count>SIZE_MAX/sizeof(uint32_t)){snprintf(action_status,sizeof(action_status),"This world is too large for the resource forecast.");return;}
 auto* states=count?(uint32_t*)malloc(count*sizeof(uint32_t)):nullptr;
 if(count && !states){snprintf(action_status,sizeof(action_status),"Not enough memory for the resource forecast.");return;}
 const alpha::WorldAccountingStage stage{{alpha::Variant::base,false,false,{0,false},0,0},0,0};
 alpha::WorldAccountingResult next{};const bool was_dirty=player.dirty();
 const auto error=alpha::world_base_accounting(*current,technologies,definitions,basic,world_rules,forecast_resources,selected_base,stage,next,states,count);
 if(error==alpha::StableIntakeError::ok && forecast_citizens_ready){
  const auto* owner=current->faction((size_t)base->faction_id);
  const alpha::BaseBudgetStage budget_stage{stage,owner->energy_credits,0,owner->player_flags};
  forecast_budget_ready=alpha::world_base_budget(*current,technologies,definitions,basic,world_rules,forecast_resources,
   forecast_citizens,facility_labels,selected_base,budget_stage,forecast_budget,states,count)==alpha::BudgetError::ok;
 }
 free(states);
 if(error!=alpha::StableIntakeError::ok){
  const char* message="Forecast unavailable for this base.";
  if(error==alpha::StableIntakeError::needs_allocation)message="This base needs worker reassignment before its resources can be forecast.";
  else if(error==alpha::StableIntakeError::unsupported_stage)message="Forecast unavailable for this base's current management.";
  else if(error==alpha::StableIntakeError::out_of_memory)message="Not enough memory for the resource forecast.";
  else if(error==alpha::StableIntakeError::overflow)message="Forecast values exceed the supported range.";
  snprintf(action_status,sizeof(action_status),"%s",message);
  printf("RESOURCE FORECAST base=%lu error=%u dirty_before=%d dirty_after=%d\n",(unsigned long)selected_base,(unsigned)error,was_dirty,player.dirty());fflush(stdout);return;
 }
 forecast=next;forecast_base=selected_base;forecast_open=true;base_open=false;
 snprintf(action_status,sizeof(action_status),"Base resource forecast. F recalculates; ESC closes. Resources have not been added.");
 printf("RESOURCE FORECAST base=%lu support=%ld food=%ld,%ld minerals=%ld,%ld energy_before_trade=%ld dirty_before=%d dirty_after=%d\n",
  (unsigned long)forecast_base,(long)forecast.support.mineral_cost,(long)forecast.nutrient.intake,(long)forecast.nutrient.surplus,
  (long)forecast.mineral.minerals.intake,(long)forecast.mineral.minerals.surplus,(long)forecast.energy.accounting.surplus,was_dirty,player.dirty());fflush(stdout);
 if(forecast_budget_ready){
  printf("BUDGET FORECAST base=%lu economy=%ld labs=%ld psych=%ld upkeep=%ld credits_after_income=%ld reserves_after_this_base=%ld dirty_before=%d dirty_after=%d\n",
   (unsigned long)forecast_base,(long)forecast_budget.allocation.allocation.economy,(long)forecast_budget.allocation.allocation.labs,
   (long)forecast_budget.allocation.allocation.psych,(long)forecast_budget.maintenance.maintenance_after,
   (long)forecast_budget.credits_after_income,(long)forecast_budget.maintenance.credits_after,was_dirty,player.dirty());fflush(stdout);
 }
}
static void draw_forecast(struct Window* window){
 if(!local_enabled || !forecast_open || forecast_base!=selected_base)return;
 const auto* current=player.world();const auto* base=current?current->base(forecast_base):nullptr;if(!base)return;
 auto* rp=window->RPort;FillPixelArray(rp,104,120,744,506,0x00071316u);char line[180];
 ui_text.draw(rp,128,160,"BASE RESOURCE FORECAST",660,true,alpha::UiInk::title);
 ui_text.draw(rp,128,192,base->name,660,true,alpha::UiInk::value);
 ui_text.draw(rp,128,220,"Calculated from current worked tiles and units.",660);
 snprintf(line,sizeof(line),"Food: intake %ld   |   used %ld   |   net %+ld",(long)forecast.nutrient.intake,(long)forecast.nutrient.consumption,(long)forecast.nutrient.surplus);ui_text.draw(rp,128,264,line,660);
 snprintf(line,sizeof(line),"Minerals: gross %ld   |   unit upkeep %ld   |   net %+ld",(long)forecast.mineral.minerals.intake,(long)forecast.support.mineral_cost,(long)forecast.mineral.minerals.surplus);ui_text.draw(rp,128,304,line,660);
 snprintf(line,sizeof(line),"Energy before trade: net %+ld",(long)forecast.energy.accounting.surplus);ui_text.draw(rp,128,352,line,660,false,alpha::UiInk::value);
 snprintf(line,sizeof(line),"Energy intake %ld   |   loss %ld   |   exports %ld",(long)forecast.energy.accounting.intake,(long)forecast.energy.accounting.inefficiency,(long)forecast.energy.accounting.consumption);ui_text.draw(rp,128,380,line,660);
 if(forecast_budget_ready){
  const auto& a=forecast_budget.allocation.allocation;
  snprintf(line,sizeof(line),"Economy %ld   |   Research %ld   |   Psych %ld",(long)a.economy,(long)a.labs,(long)a.psych);ui_text.draw(rp,128,426,line,660,false,alpha::UiInk::value);
  snprintf(line,sizeof(line),"Facility upkeep: %ld   |   balance before trade: %+ld",(long)forecast_budget.maintenance.maintenance_after,
   (long)(a.economy-forecast_budget.maintenance.maintenance_after));ui_text.draw(rp,128,458,line,660);
  ui_text.draw(rp,128,494,"Uses current energy settings; excludes stockpile production.",660);
 }else ui_text.draw(rp,128,442,"Energy allocation forecast is unavailable for this base.",660);
 ui_text.draw(rp,128,550,"Forecast only; growth and drone effects remain unresolved.",660);
 ui_text.draw(rp,128,586,"ESC: close   |   F: recalculate",660);
}
#endif
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
 for(size_t i=0;i<world().game().base_count;++i){const auto& b=*world().base(i);if(b.y!=map_y)continue;int sx,sy;if(position(b.x,b.y,sx,sy))base_sprite(sx,sy,b);}
 for(size_t i=0;i<world().game().vehicle_count;++i){const auto& v=*world().vehicle(i);if(v.y!=map_y)continue;int sx,sy;if(!position(v.x,v.y,sx,sy))continue;int row=-1,col=0;uint32_t morale;
  if(alpha::world_effective_morale(world(),definitions,technologies,i,true,morale)==alpha::LogicError::ok)col=(int)morale;
  switch(v.unit_id){case 7:row=0;col=0;break;case 8:row=3;break;case 9:row=4;break;case 10:row=7;col=0;break;case 15:if(world().game().data_version>=11)row=5;break;}
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
 snprintf(line,sizeof(line),"%.64s | Turn %ld | %lu x %lu | %lu units, %lu bases",source_label,(long)world().game().preferences.CurrentTurn,(unsigned long)world().game().map.info.width,(unsigned long)world().game().map.info.height,(unsigned long)world().game().vehicle_count,(unsigned long)world().game().base_count);text(rp,15,42,line);
 text(rp,15,62,local_enabled?"Local actions | Keypad: move | P: production | F: forecast | V: save | R: reload | No End Turn":"Read-only original world | Basic infantry CVR profile; other models pending | WASD scroll | TAB select | +/- zoom | Q quit");
 for(size_t i=0;i<world().game().base_count;++i){const auto& b=*world().base(i);int sx,sy;if(position(b.x,b.y,sx,sy) && sx>=30 && sx<W-180 && sy>0 && sy<H-32){snprintf(line,sizeof(line),"[Base] %.24s",b.name);ui_text.draw(rp,12+sx-24,70+sy+24,line,W-(12+sx-24),false,alpha::UiInk::map_label);}}
 for(size_t i=0;i<world().game().vehicle_count;++i){const auto& v=*world().vehicle(i);int sx,sy;if(!position(v.x,v.y,sx,sy) || sy<0 || sy>=H)continue;
  const auto& p=*world().prototype((size_t)v.unit_id);bool native=(v.unit_id>=7 && v.unit_id<=10) || infantry_profile(v);
  if(!native && sx>=30 && sx<W-180){snprintf(line,sizeof(line),"%.16s [CVR]",p.name[0]?p.name:"Saved design");text(rp,12+sx-25,70+sy-16,line);}
  if(i==selected){Move(rp,12+sx,70+sy-14);Draw(rp,12+sx+29,70+sy);Draw(rp,12+sx,70+sy+14);Draw(rp,12+sx-29,70+sy);Draw(rp,12+sx,70+sy-14);}
 }
 draw_hud(window);draw_base_details(window);
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 draw_forecast(window);
#endif
 printf("DRAW selected=%lu camera=%d,%d zoom=%d\n",(unsigned long)selected,camera_x,camera_y,zoom);fflush(stdout);
}

#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
static bool leave_local(){
 if(local_enabled && player.dirty() && discard_pending!='q'){discard_pending='q';snprintf(action_status,sizeof(action_status),"Unsaved changes. V saves a new checkpoint; Q again discards and exits.");return false;}
 return true;
}
static void save_local(){
 const size_t n=player.checkpoint_size();auto* bytes=(uint8_t*)malloc(n);
 if(!bytes){snprintf(action_status,sizeof(action_status),"Save failed: not enough memory.");return;}
 const auto e=player.checkpoint(bytes,n);
 if(e!=alpha::LocalSnapshotError::ok){free(bytes);snprintf(action_status,sizeof(action_status),"Save refused: %s",alpha::local_snapshot_error_name(e));return;}
 // CreateDir exclusively reserves a fresh slot. Existing files/directories
 // are never opened for replacement. Incomplete slots cannot pass resume CRC.
 struct DateStamp stamp;DateStamp(&stamp);char drawer[200];BPTR lock=0;
 for(unsigned attempt=0;attempt<100 && !lock;++attempt){snprintf(drawer,sizeof(drawer),"PROGDIR:AlphaLocal-%ld-%ld-%ld-%u",(long)stamp.ds_Days,(long)stamp.ds_Minute,(long)stamp.ds_Tick,attempt);lock=CreateDir((STRPTR)drawer);}
 if(!lock){free(bytes);snprintf(action_status,sizeof(action_status),"Could not create a new save slot. Changes remain unsaved.");return;}
 UnLock(lock);char path[256];snprintf(path,sizeof(path),"%s/session.aclocal",drawer);FILE* f=fopen(path,"wb");bool ok=false;
 if(f){ok=fwrite(bytes,1,n,f)==n;if(fclose(f))ok=false;}free(bytes);
 if(ok){snprintf(last_checkpoint,sizeof(last_checkpoint),"%s",path);player.saved();discard_pending=false;snprintf(action_status,sizeof(action_status),"Saved %.150s",drawer);}
 else snprintf(action_status,sizeof(action_status),"Save write failed. Changes remain unsaved; partial slot retained.");
}
static bool local_key(UWORD code){
 if(!local_enabled)return false;
 if(code=='f' || code=='F'){discard_pending=0;open_forecast();return true;}
 if(code==27 && forecast_open){forecast_open=false;snprintf(action_status,sizeof(action_status),"Resource forecast closed. F opens it again.");return true;}
 forecast_open=false;
 if(code=='v' || code=='V'){save_local();return true;}
 if(code=='r' || code=='R'){
  if(!last_checkpoint[0]){snprintf(action_status,sizeof(action_status),"No checkpoint saved in this session. Open an earlier checkpoint from the menu.");return true;}
  if(player.dirty() && discard_pending!='r'){discard_pending='r';snprintf(action_status,sizeof(action_status),"Unsaved changes. R again reloads the last checkpoint; V saves them first.");return true;}
  size_t n=0;auto* bytes=read_file(last_checkpoint,n);auto e=bytes?player.resume(bytes,n,player_rules,player_rules_length):alpha::LocalOrderError::bad_world;free(bytes);
  if(e==alpha::LocalOrderError::ok){visible_world=player.world();base_open=false;select_local_actors();discard_pending=false;}
  snprintf(action_status,sizeof(action_status),"Reload: %s",alpha::local_order_error_name(e));return true;
 }
 discard_pending=false;
 if(code=='p' || code=='P'){
  auto e=player.next_facility((int32_t)selected_base);base_open=true;
  snprintf(action_status,sizeof(action_status),e==alpha::LocalOrderError::ok?"P selects another available facility. ENTER confirms; ESC cancels.":"Production unavailable: %s",alpha::local_order_error_name(e));return true;
 }
 if(code==13){alpha::FacilityOrderReceipt out{};auto e=player.confirm_facility(out);snprintf(action_status,sizeof(action_status),"Production: %s",alpha::local_order_error_name(e));return true;}
 if(code==27 || code=='b' || code=='B'){player.cancel_facility();return false;}
 int dx=0,dy=0;
 switch(code){case '7':dx=-1;dy=-1;break;case '8':dy=-2;break;case '9':dx=1;dy=-1;break;case '4':dx=-2;break;case '6':dx=2;break;case '1':dx=-1;dy=1;break;case '2':dy=2;break;case '3':dx=1;dy=1;break;default:return false;}
 if(base_open){snprintf(action_status,sizeof(action_status),"Close base details with ESC before moving.");return true;}
 alpha::LocalStepReceipt out{};auto e=player.step(selected,dx,dy,out);
 if(e==alpha::LocalOrderError::ok){snprintf(action_status,sizeof(action_status),"Move: %s (cost %lu)",alpha::step_status_name(out.result.status),(unsigned long)out.result.price);if(out.changed){camera_x=world().vehicle(selected)->x;camera_y=world().vehicle(selected)->y;}}
 else snprintf(action_status,sizeof(action_status),"Move unavailable: %s",alpha::local_order_error_name(e));
 return true;
}
#endif
int alpha_original_view(int argc,char** argv){
 if((argc!=6 && argc!=7) || (strcmp(argv[1],"base") && strcmp(argv[1],"expansion"))){puts("Usage: alpha-original-view base|expansion RULES SCENARIO texture.pcx Units.pcx");return 20;}
 set_source_label(argv[3]);
 local_enabled=false;visible_world=&preview_world;action_status[0]=0;
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 PlayerViewScope player_scope;
 forecast_open=false;forecast_resources_ready=false;forecast_citizens_ready=false;forecast_budget_ready=false;
 player.close();free(player_rules);player_rules=nullptr;player_rules_length=0;last_checkpoint[0]=0;discard_pending=false;
 if(argc==7){
  if(strcmp(argv[6],"--local") || strcmp(argv[1],"base"))return 20;
  size_t rn=0,sn=0;auto* rules=read_file(argv[2],rn);auto* save=read_file(argv[3],sn);
  if(!rules || !save){free(rules);free(save);return 20;}
  const auto e=sn>=8 && !memcmp(save,"ACLOCAL1",8)?player.resume(save,sn,rules,rn):player.load(save,sn,rules,rn,0x12345678u);
  free(save);
  if(e!=alpha::LocalOrderError::ok){printf("LOCAL LOAD %s\n",alpha::local_order_error_name(e));free(rules);return 20;}
  player_rules=rules;player_rules_length=rn;local_enabled=true;visible_world=player.world();
  snprintf(action_status,sizeof(action_status),"Local actions: keypad moves; P facility; ENTER confirm; F forecast; V save; R reload last save.");
 }
#else
 if(argc==7)return 20;
#endif
 size_t n;auto* p=read_file(argv[2],n);if(!p)return 20;auto variant=strcmp(argv[1],"base")==0?alpha::Variant::base:alpha::Variant::alien_crossfire;
 bool ok=alpha::read_technologies(p,n,technologies)==alpha::Error::ok && alpha::read_unit_definitions(p,n,technologies,variant,definitions)==alpha::Error::ok && alpha::read_basic_rules(p,n,technologies,variant,basic,world_rules)==alpha::Error::ok;
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 if(local_enabled)forecast_resources_ready=alpha::read_resource_info(p,n,forecast_resources)==alpha::Error::ok;
 if(local_enabled)forecast_citizens_ready=alpha::read_citizens(p,n,technologies,forecast_citizens)==alpha::Error::ok;
#endif
 free(p);if(!ok)return 20;
 if(!local_enabled){p=read_file(argv[3],n);if(!p)return 20;ok=preview_world.load(p,n,0x12345678u)==alpha::WorldLoadError::ok;free(p);if(!ok || (world().game().data_version<11)!=(variant==alpha::Variant::base))return 20;}
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 if(local_enabled && alpha::read_facility_definitions(player_rules,player_rules_length,technologies,alpha::Variant::base,facility_labels)!=alpha::Error::ok)return 20;
#endif
 if(!load_image(argv[4],terrain) || !load_image(argv[5],sprites))return 20;
 p=read_file("console.pcx",n);if(!p)return 20;ok=console_image.load(p,n);free(p);if(!ok || console_image.width()!=800 || console_image.height()!=257)return 20;
 selected=selected_base=0;base_open=false;
 if(local_enabled)select_local_actors();
 bool needed[8]{};
 for(size_t i=0;i<world().game().base_count;++i){int f=world().base(i)->faction_id;if(f<0 || f>=8)return 20;needed[f]=true;}
 for(unsigned f=0;f<8;++f)if(needed[f]){char name[32];unsigned len=0;const auto& def=*world().faction_definition(f);
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
 if(world().game().vehicle_count){camera_x=world().vehicle(selected)->x;camera_y=world().vehicle(selected)->y;}else {camera_x=(int)world().game().map.info.width/2;camera_y=(int)world().game().map.info.height/2;}
 IntuitionBase=(struct IntuitionBase*)OpenLibrary((STRPTR)"intuition.library",39);GfxBase=(struct GfxBase*)OpenLibrary((STRPTR)"graphics.library",39);CyberGfxBase=OpenLibrary((STRPTR)"cybergraphics.library",41);
 struct Window* window=nullptr;
 char window_title[128];snprintf(window_title,sizeof(window_title),"Alpha Centauri - %s",source_label);
 if(IntuitionBase && GfxBase && CyberGfxBase)window=OpenWindowTags(NULL,WA_Title,(ULONG)window_title,WA_Width,952,WA_Height,864,WA_Flags,WFLG_CLOSEGADGET|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_ACTIVATE|WFLG_SMART_REFRESH,WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_VANILLAKEY|IDCMP_MOUSEBUTTONS|IDCMP_REFRESHWINDOW,TAG_DONE);
 int rc=20;
 if(!window)fprintf(stderr,"Cannot open world window (952x864): intuition=%d graphics=%d cybergraphics=%d\n",IntuitionBase!=nullptr,GfxBase!=nullptr,CyberGfxBase!=nullptr);
 if(window){draw(window);bool quit=false;while(!quit){WaitPort(window->UserPort);struct IntuiMessage* m;
  while((m=(struct IntuiMessage*)GetMsg(window->UserPort))){ULONG kind=m->Class;UWORD code=m->Code;WORD mouse_x=m->MouseX,mouse_y=m->MouseY;ReplyMsg((struct Message*)m);
   if(kind==IDCMP_CLOSEWINDOW){
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    quit=leave_local();if(!quit)draw(window);
#else
    quit=true;
#endif
   }
   else if(kind==IDCMP_REFRESHWINDOW){BeginRefresh(window);draw(window);EndRefresh(window,TRUE);}
   else if(kind==IDCMP_MOUSEBUTTONS && code==SELECTDOWN){
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    discard_pending=0;forecast_open=false;
#endif
    if(mouse_x>=12+HUD_X+563 && mouse_x<12+HUD_X+787 && mouse_y>=HUD_Y+52 && mouse_y<HUD_Y+226){camera_x=(int)((uint32_t)(mouse_x-12-HUD_X-563)*world().game().map.info.width/224);camera_y=(int)((uint32_t)(mouse_y-HUD_Y-52)*world().game().map.info.height/174);}
    else for(size_t i=0;i<world().game().base_count;++i){int x,y;const auto&b=*world().base(i);if(position(b.x,b.y,x,y) && abs(mouse_x-12-x)<24*zoom && abs(mouse_y-70-y)<24*zoom){selected_base=i;base_open=true;
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
     player.cancel_facility();discard_pending=false;
#endif
     break;}}
    draw(window);
   }
   else if(kind==IDCMP_VANILLAKEY){
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
    if(code=='q' || code=='Q'){quit=leave_local();if(!quit)draw(window);continue;}
    if(local_key(code)){printf("LOCAL ACTION key=%u status=%s dirty=%d\n",(unsigned)code,action_status,player.dirty());fflush(stdout);draw(window);continue;}
#endif
    switch(code){case 't':case 'T':{int rc=alpha_technology_browser(argv[2]);printf("WORLD TECHNOLOGY RC %d\n",rc);fflush(stdout);ActivateWindow(window);}break;case 'q':case 'Q':quit=true;break;case 'w':camera_y-=2;break;case 's':camera_y+=2;break;case 'a':camera_x=wrap(camera_x-2);break;case 'd':camera_x=wrap(camera_x+2);break;case 'b':if(world().game().base_count){selected_base=(selected_base+1)%world().game().base_count;camera_x=world().base(selected_base)->x;camera_y=world().base(selected_base)->y;}break;case 'i':if(world().game().base_count)base_open=!base_open;break;case 27:base_open=false;break;case '+':case '=':zoom=2;break;case '-':zoom=1;break;case 9:if(world().game().vehicle_count)selected=(selected+1)%world().game().vehicle_count;break;case 'c':if(world().game().vehicle_count){camera_x=world().vehicle(selected)->x;camera_y=world().vehicle(selected)->y;}break;}
    if(camera_x<0)camera_x=0;
    if(camera_x>=(int)world().game().map.info.width)camera_x=(int)world().game().map.info.width-1;
    if(camera_y<0)camera_y=0;
    if(camera_y>=(int)world().game().map.info.height)camera_y=(int)world().game().map.info.height-1;
    draw(window);}
  }
 }CloseWindow(window);rc=0;}
 if(CyberGfxBase)CloseLibrary(CyberGfxBase);
 if(GfxBase)CloseLibrary((struct Library*)GfxBase);
 if(IntuitionBase)CloseLibrary((struct Library*)IntuitionBase);
#ifdef ALPHA_LOCAL_PLAYER_CANDIDATE
 player.close();visible_world=&preview_world;local_enabled=false;free(player_rules);player_rules=nullptr;player_rules_length=0;
#endif
 free(frame);free(hud_frame);ui_text.close();printf("ORIGINAL VIEW RC %d\n",rc);return rc;
}

#ifndef ALPHA_EMBEDDED_VIEW
int main(int argc,char** argv){return alpha_original_view(argc,argv);}
#endif
