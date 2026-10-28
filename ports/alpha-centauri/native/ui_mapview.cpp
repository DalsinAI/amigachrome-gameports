/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "ui_mapview.h"
#include "cvr_palette.h"
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
namespace alpha { namespace {
MapTiles tiles_of(const OriginalWorld& w){const auto& i=w.game().map.info;return {i.width,i.height,i.flat,w.tile(0),i.tile_count};}
void sample(const PcxImage* t,unsigned x,unsigned y,uint8_t out[3],uint8_t r,uint8_t g,uint8_t b){
 const uint8_t* c=t?t->rgb(x,y):nullptr;if(c)memcpy(out,c,3);else{out[0]=r;out[1]=g;out[2]=b;}
}
}
MapViewExtras::MapViewExtras():recipes_(nullptr),ter1_(nullptr),global_{},tints_{},tint_mask_(0),palette_ready_(false),borders_{},border_mask_(0),viewer_(-1){}
MapViewExtras::~MapViewExtras(){close();}
void MapViewExtras::close(){vehicles_.clear();free(recipes_);recipes_=nullptr;if(ter1_){ter1_->~PcxImage();free(ter1_);}ter1_=nullptr;tint_mask_=border_mask_=0;palette_ready_=false;viewer_=-1;}
void MapViewExtras::open(const OriginalWorld& w,const PcxImage* const art[8],Reader reader,void* context){
 close();log_.clear();
 const int32_t turn=w.game().preferences.CurrentTurn;char line[120];
 size_t n=0;uint8_t* bytes=reader?reader(context,"palette.pcx",n):nullptr;
 if(!bytes && reader)bytes=reader(context,"programs/palette.pcx",n);
 if(bytes){PcxImage palette;palette_ready_=palette.load(bytes,n) && build_cvr_default_global_palette(palette.palette(),768,global_,sizeof(global_));free(bytes);}
 for(unsigned f=0;f<8;++f){if(!art || !art[f])continue;
  if(faction_border_colour(*art[f],borders_[f]))border_mask_|=(uint8_t)(1u<<f);
  if(palette_ready_ && cvr_faction_tint(art[f]->pixels(),art[f]->width(),art[f]->height(),global_,tints_[f]))tint_mask_|=(uint8_t)(1u<<f);}
 snprintf(line,sizeof(line),palette_ready_?"Faction unit colours ready for %u factions.":"palette.pcx not found: units keep model colours.",(unsigned)__builtin_popcount(tint_mask_));
 log_.add(turn,MessageKind::info,line);
 bytes=reader?reader(context,"cvr_units.txt",n):nullptr;
 if(bytes){recipes_=(CvrRecipeTable*)calloc(1,sizeof(CvrRecipeTable));
  if(!recipes_ || !parse_cvr_recipe_table((const char*)bytes,n,*recipes_)){free(recipes_);recipes_=nullptr;log_.add(turn,MessageKind::warning,"cvr_units.txt rejected; built-in unit recipes only.");}
  else{snprintf(line,sizeof(line),"cvr_units.txt: %u extra unit recipes.",recipes_->count);log_.add(turn,MessageKind::info,line);}
  free(bytes);}
 bytes=reader?reader(context,"ter1.pcx",n):nullptr;
 if(bytes){void* mem=malloc(sizeof(PcxImage));ter1_=mem?new(mem) PcxImage():nullptr;
  if(ter1_ && !(ter1_->load(bytes,n) && ter1_->width()>=1024 && ter1_->height()>=768)){ter1_->~PcxImage();free(ter1_);ter1_=nullptr;}free(bytes);}
 log_.add(turn,MessageKind::info,ter1_?"Improvement art from ter1.pcx.":"ter1.pcx not found: improvements use simple markers.");
 vehicles_.configure(reader,context,recipes_,palette_ready_?global_:nullptr,tints_,tint_mask_);
}
bool MapViewExtras::tile_visible(const OriginalWorld& w,int x,int y)const{
 const OriginalMapTile* t=map_tile_at(tiles_of(w),x,y);return t && map_tile_explored(*t,viewer_);
}
bool MapViewExtras::draw_overlays(uint8_t* frame,unsigned width,unsigned height,const OriginalWorld& w,const PcxImage* terrain,int cx,int cy,int zoom,MapOverlayStats* stats){
 MapOverlayStyle style{};style.texture=terrain;style.ter1=ter1_;memcpy(style.borders,borders_,sizeof(borders_));style.border_mask=border_mask_;style.viewer=viewer_;style.layers=layer_all;
 return draw_map_overlays(frame,(size_t)width*height*3,tiles_of(w),{cx,cy,zoom,width,height},style,stats);
}
bool MapViewExtras::has_vehicle_model(const OriginalWorld& w,size_t index){
 const OriginalVehicle* v=w.vehicle(index);if(!v || v->unit_id<0)return false;
 const OriginalPrototype* p=w.prototype((size_t)v->unit_id);const OriginalFactionDefinition* f=w.faction_definition((size_t)v->faction_id);
 if(!p || !f || v->faction_id>7 || v->rotate_angle< -1 || v->rotate_angle>7 || w.game().data_version>=11 || (f->rule_flags&0x80))return false;
 return vehicles_.get(p->chassis_id,p->weapon_id,p->armor_id,p->reactor_id,p->ability_flags)!=nullptr;
}
bool MapViewExtras::draw_vehicle(uint8_t* frame,unsigned width,unsigned height,const OriginalWorld& w,size_t index,int sx,int sy,int zoom){
 if(!has_vehicle_model(w,index) || sx<0 || sy<0 || sx>=(int)width || sy>=(int)height)return false;
 const OriginalVehicle* v=w.vehicle(index);const OriginalPrototype* p=w.prototype((size_t)v->unit_id);
 CvrVehicle* model=vehicles_.get(p->chassis_id,p->weapon_id,p->armor_id,p->reactor_id,p->ability_flags);
 const unsigned facing=v->rotate_angle<0?0:(unsigned)v->rotate_angle&7u;
 return model && model->draw(frame,(size_t)width*height*3,width,height,sx,sy,(unsigned)zoom,facing,(unsigned)v->faction_id);
}
bool MapViewExtras::draw_minimap(uint8_t* frame,unsigned width,unsigned height,const UiRect& r,const OriginalWorld& w,const PcxImage* terrain,int cx,int cy,int zoom,unsigned vw,unsigned vh){
 MinimapStyle s{};s.viewer=viewer_;
 sample(terrain,29,143,s.land,96,120,64);sample(terrain,29,29,s.rocky,128,116,100);sample(terrain,308,107,s.shelf,48,96,150);
 sample(terrain,308,164,s.ocean,24,48,110);sample(terrain,308,544,s.fungus,150,70,140);
 s.unexplored[0]=s.unexplored[1]=s.unexplored[2]=0;s.view[0]=s.view[1]=s.view[2]=240;
 memcpy(s.factions,borders_,sizeof(borders_));s.faction_mask=border_mask_;
 const size_t count=w.game().base_count+w.game().vehicle_count;
 MinimapMarker* markers=count?(MinimapMarker*)malloc(count*sizeof(MinimapMarker)):nullptr;size_t used=0;
 if(count && !markers)return false;
 for(size_t i=0;i<w.game().vehicle_count;++i){const auto* v=w.vehicle(i);markers[used++]={v->x,v->y,v->faction_id,false};}
 for(size_t i=0;i<w.game().base_count;++i){const auto* b=w.base(i);markers[used++]={b->x,b->y,b->faction_id,true};}
 const MapCamera cam{cx,cy,zoom,vw,vh};
 const bool ok=alpha::draw_minimap({frame,(size_t)width*height*3,width,height},r,tiles_of(w),s,markers,used,&cam);
 free(markers);return ok;
}
bool MapViewExtras::minimap_click(const OriginalWorld& w,const UiRect& r,int px,int py,int& x,int& y)const{return minimap_to_map(tiles_of(w),r,px,py,x,y);}
size_t MapViewExtras::draw_base_window(uint8_t* frame,unsigned width,unsigned height,const UiRect& r,const OriginalWorld& w,size_t base,
 const FacilityDefinitions* facilities,const BaseWindowForecast* forecast,UiTextItem* items,size_t capacity){
 BaseWindowModel* model=(BaseWindowModel*)malloc(sizeof(BaseWindowModel));if(!model)return 0;
 size_t n=0;
 if(build_base_window(w,base,facilities,forecast,viewer_,*model))n=render_base_window(*model,{frame,(size_t)width*height*3,width,height},r,items,capacity);
 free(model);return n;
}
size_t MapViewExtras::draw_messages(uint8_t* frame,unsigned width,unsigned height,const UiRect& r,UiTextItem* items,size_t capacity){
 return layout_message_panel(log_,{frame,(size_t)width*height*3,width,height},r,18,items,capacity);
}
void MapViewExtras::base_label(const OriginalBase& b,char* out,size_t size){snprintf(out,size,"%.24s (%d)",b.name,(int)b.pop_size);}
}
