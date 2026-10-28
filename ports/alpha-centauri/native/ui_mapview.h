/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UI_MAPVIEW_H
#define ALPHA_UI_MAPVIEW_H
#include "cvr_vehicle.h"
#include "map_overlays.h"
#include "ui_basewin.h"
#include "ui_messages.h"
#include "ui_minimap.h"
namespace alpha {
/* Platform-free state the original-world window adds through small hooks:
 * owner art (palette.pcx, ter1.pcx, cvr_units.txt), faction colours, fog
 * viewer, assembled CVR vehicles, overlays, minimap, message log and base
 * window. All drawing is into the window's RGB24 frames; text comes back as
 * UiTextItem lists. Asset reads go through the caller's reader. */
class MapViewExtras {
public:
 typedef uint8_t* (*Reader)(void* context,const char* name,size_t& size);
 MapViewExtras();~MapViewExtras();
 /* faction_art[f] may be null. Missing optional assets only disable the
  * corresponding feature; each outcome is logged to the message log. */
 void open(const OriginalWorld&,const PcxImage* const faction_art[8],Reader reader,void* context);
 void close();
 /* -1 omniscient, else a faction id whose explored tiles are shown. */
 void set_viewer(int viewer){viewer_=viewer;}
 int viewer()const{return viewer_;}
 bool tile_visible(const OriginalWorld&,int x,int y)const;
 bool draw_overlays(uint8_t* frame,unsigned width,unsigned height,const OriginalWorld&,const PcxImage* terrain,int camera_x,int camera_y,int zoom,MapOverlayStats* stats=nullptr);
 /* True if an assembled CVR vehicle for this record was drawn. */
 bool draw_vehicle(uint8_t* frame,unsigned width,unsigned height,const OriginalWorld&,size_t vehicle,int sx,int sy,int zoom);
 bool has_vehicle_model(const OriginalWorld&,size_t vehicle);
 bool draw_minimap(uint8_t* frame,unsigned width,unsigned height,const UiRect&,const OriginalWorld&,const PcxImage* terrain,int camera_x,int camera_y,int zoom,unsigned view_width,unsigned view_height);
 bool minimap_click(const OriginalWorld&,const UiRect&,int px,int py,int& x,int& y)const;
 size_t draw_base_window(uint8_t* frame,unsigned width,unsigned height,const UiRect&,const OriginalWorld&,size_t base,
  const FacilityDefinitions* facilities,const BaseWindowForecast* forecast,UiTextItem* items,size_t capacity);
 size_t draw_messages(uint8_t* frame,unsigned width,unsigned height,const UiRect&,UiTextItem* items,size_t capacity);
 MessageLog& messages(){return log_;}
 bool faction_colours()const{return palette_ready_;}
 /* "Name (pop)" map label for a base. */
 static void base_label(const OriginalBase&,char* out,size_t size);
private:
 MapViewExtras(const MapViewExtras&)=delete;MapViewExtras& operator=(const MapViewExtras&)=delete;
 CvrVehicleSet vehicles_;CvrRecipeTable* recipes_;PcxImage* ter1_;
 uint8_t global_[768];uint8_t tints_[8][3];uint8_t tint_mask_;bool palette_ready_;
 uint8_t borders_[8][3];uint8_t border_mask_;int viewer_;MessageLog log_;
};
}
#endif
