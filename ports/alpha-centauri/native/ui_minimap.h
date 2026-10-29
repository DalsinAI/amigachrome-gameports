/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_UI_MINIMAP_H
#define ALPHA_UI_MINIMAP_H
#include "map_overlays.h"
#include "ui_panel.h"
namespace alpha {
struct MinimapStyle {
 int viewer;                     /* -1 omniscient, faction 1..7 applies fog */
 uint8_t land[3],rocky[3],shelf[3],ocean[3],fungus[3],unexplored[3],view[3];
 uint8_t factions[8][3];uint8_t faction_mask;
};
struct MinimapMarker { int x,y,faction;bool base; };
struct MinimapStats { uint32_t pixels,explored,owned,bases,units,view_pixels; };
/* Whole-map thumbnail into rect of frame: each pixel samples the tile under it
 * (odd parity steps to the neighbour column), owned land is tinted 3:1 toward
 * the owner colour, markers on explored tiles become 3x3 bases / 1px units,
 * and the camera rectangle is outlined (wrapping horizontally). */
bool draw_minimap(const UiFrame&,const UiRect&,const MapTiles&,const MinimapStyle&,
 const MinimapMarker* markers,size_t marker_count,const MapCamera* camera,MinimapStats* stats=nullptr);
/* Inverse mapping for clicks inside rect; false outside. */
bool minimap_to_map(const MapTiles&,const UiRect&,int px,int py,int& x,int& y);
}
#endif
