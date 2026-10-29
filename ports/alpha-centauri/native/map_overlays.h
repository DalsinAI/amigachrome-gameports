/* SPDX-License-Identifier: GPL-3.0-or-later
 * Map tile bit meanings from Thinker engine_enums.h (MIT, pin dabe62f2;
 * THINKER-LICENSE.txt). texture.pcx/ter1.pcx cell coordinates are asset layout
 * facts cross-checked against GLSMAC Consts.h at 0fe9ef16; no GLSMAC code used.
 */
#ifndef ALPHA_MAP_OVERLAYS_H
#define ALPHA_MAP_OVERLAYS_H
#include "original_map.h"
#include "pcx_image.h"
#include <stddef.h>
#include <stdint.h>
namespace alpha {
namespace map_bit {
constexpr uint32_t base=0x1,unit=0x2,road=0x4,magtube=0x8,mine=0x10,fungus=0x20,solar=0x40,river=0x80,
 bunker=0x800,monolith=0x2000,farm=0x8000,airbase=0x40000,soil_enricher=0x80000,forest=0x200000,
 condenser=0x400000,mirror=0x800000,borehole=0x1000000,supply_pod=0x10000000,sensor=0x80000000u;
}
/* Borrowed owned tiles in original half-width order: (x,y) with even x+y is
 * tile (y*width+x)/2. Non-flat maps wrap horizontally. */
struct MapTiles { uint32_t width,height;bool flat;const OriginalMapTile* tiles;size_t count; };
const OriginalMapTile* map_tile_at(const MapTiles&,int x,int y);
/* Flat isometric view: tile centre at frame centre + (dx*32*zoom,(y-cy)*16*zoom). */
struct MapCamera { int x,y,zoom;uint32_t width,height; };
bool map_screen_position(const MapTiles&,const MapCamera&,int x,int y,int& sx,int& sy);
/* Viewer -1 sees everything. Faction 1..7 sees only tiles whose visibility bit
 * is set, with the improvement bits recorded when it last saw them. */
bool map_tile_explored(const OriginalMapTile&,int viewer);
uint32_t map_tile_items(const OriginalMapTile&,int viewer);
bool map_tile_ocean(const OriginalMapTile&);
enum MapLayer : unsigned { layer_vegetation=1,layer_rivers=2,layer_roads=4,layer_improvements=8,layer_borders=16,layer_fog=32,layer_all=63 };
struct MapOverlayStyle {
 const PcxImage* texture;  /* optional texture.pcx (forest, fungus, farm cells) */
 const PcxImage* ter1;     /* optional ter1.pcx improvement sprites */
 uint8_t borders[8][3];uint8_t border_mask;int viewer;unsigned layers;
};
struct MapOverlayStats {
 uint32_t tiles,unexplored,forest,fungus,farms,rivers,river_links,roads,road_links,tubes,tube_links,
  mines,solar,sensors,other_improvements,border_edges,art_cells,glyphs;
};
/* Paint overlays for every tile visible in the camera over an RGB24 frame of
 * camera.width x camera.height whose terrain is already drawn. Rivers, roads,
 * mag tubes and borders are drawn as connection strokes (new presentation, not
 * the original atlas variant selection); farms, forest and fungus use
 * texture.pcx cells, other improvements ter1.pcx cells when given and simple
 * glyphs otherwise. Fails without writing on invalid input. */
bool draw_map_overlays(uint8_t* frame,size_t capacity,const MapTiles&,const MapCamera&,const MapOverlayStyle&,MapOverlayStats* stats=nullptr);
/* Border/territory colour from faction art (pixel 161,749 per GLSMAC's layout notes). */
bool faction_border_colour(const PcxImage& faction_art,uint8_t rgb[3]);
}
#endif
