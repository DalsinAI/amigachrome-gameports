/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_VEHICLE_H
#define ALPHA_CVR_VEHICLE_H
#include "cvr_model.h"
#include <stddef.h>
#include <stdint.h>
namespace alpha {
constexpr unsigned cvr_vehicle_max_parts=12;
/* One appended component file. Painted parts are the ones the original remaps
 * through a faction shade table; all other parts keep their own shade table. */
struct CvrVehiclePart { const void* data;size_t size;bool painted; };
/* Presentation profile of an assembled design. scale is the unit's normal-zoom
 * scale factor (infantry 1.25, speeder 1.0); root_x the root translation along
 * the first object axis; body_frames the frames the first part must provide. */
struct CvrVehicleProfile { float scale,root_x;unsigned body_frames,voxel_limit; };
/* A named assembly: up to twelve lowercase file stems in original append order. */
struct CvrRecipe {
 unsigned count;char names[cvr_vehicle_max_parts][24];bool painted[cvr_vehicle_max_parts];
 CvrVehicleProfile profile;
};
/* Generic chassis/weapon/armour/reactor assembly on one strict depth surface.
 * The projection, lighting and pixel stage are the same as the qualified
 * infantry/speeder profiles. Colour has two modes:
 *  - model: each part shows its own embedded RGB (the existing profiles);
 *  - global: the pixel stage writes global-palette indices; painted parts of a
 *    faction with a tint use build_cvr_faction_palette() tables built from the
 *    part's own neutral shade table, everything else uses that neutral table.
 * Facing/zoom/faction RGBA caches are lazy and capped at 4 MiB per assembly. */
class CvrVehicle {
public:
 CvrVehicle();~CvrVehicle();
 bool load(const CvrVehiclePart* parts,size_t count,const CvrVehicleProfile& profile);
 /* global_rgb: 768 bytes or null (model colours). tints: eight RGB triplets;
  * bit f of tint_mask marks faction f as having a valid tint. Drops caches. */
 bool set_colours(const uint8_t* global_rgb,const uint8_t (*tints)[3],uint8_t tint_mask);
 bool draw(uint8_t* rgb,size_t capacity,unsigned width,unsigned height,int center_x,int foot_y,unsigned zoom,unsigned facing,unsigned faction=0);
 bool ready()const{return ready_;}
 unsigned parts()const{return parts_;}
 size_t cache_bytes()const{return cache_bytes_;}
private:
 CvrVehicle(const CvrVehicle&)=delete;CvrVehicle&operator=(const CvrVehicle&)=delete;
 void drop_caches();
 const uint8_t* table(unsigned part,unsigned faction);
 CvrModel models_[cvr_vehicle_max_parts];bool painted_[cvr_vehicle_max_parts];
 unsigned parts_;CvrVehicleProfile profile_;uint8_t* lattice_;bool ready_;
 bool global_;uint8_t global_rgb_[768];uint8_t tints_[8][3];uint8_t tint_mask_;
 uint8_t* neutral_[cvr_vehicle_max_parts];uint8_t* faction_[8];
 uint8_t* cache_[8][16];size_t cache_bytes_;
};
/* Built-in recipes recovered with the existing qualified profiles: base-game
 * basic human infantry (0/0/0/1) and speeder (1/0/0/1) with no abilities. */
bool cvr_builtin_recipe(int chassis,int weapon,int armor,int reactor,uint32_t abilities,CvrRecipe& out);
/* Owner-supplied recipe table (cvr_units.txt in the game folder). One design per
 * line: "chassis weapon armour reactor scale root_x body_frames voxel_limit :
 * stem stem* ...", '#' comments, stems [a-z0-9_]{1,20}, '*' marks painted.
 * Duplicate designs, more than 64 lines or any malformed line reject the file. */
struct CvrRecipeTable { unsigned count;struct Entry{int chassis,weapon,armor,reactor;CvrRecipe recipe;} entries[64]; };
bool parse_cvr_recipe_table(const char* text,size_t size,CvrRecipeTable& out);
/* Table entries first (abilities must be zero), then built-ins. */
bool find_cvr_recipe(const CvrRecipeTable* table,int chassis,int weapon,int armor,int reactor,uint32_t abilities,CvrRecipe& out);
/* Faction vehicle tint from raw faction-art indices (width x height, 8-bit):
 * pixel (435,745)+10 (byte wrap) indexes the global palette, unless it or the
 * intensity pixel (161,757) is the raw transparent 255 marker. */
bool cvr_faction_tint(const uint8_t* indices,uint32_t width,uint32_t height,const uint8_t* global_rgb,uint8_t tint[3]);
/* Bounded design-keyed set of loaded assemblies for a map view. */
class CvrVehicleSet {
public:
 typedef uint8_t* (*Reader)(void* context,const char* file_name,size_t& size);
 CvrVehicleSet();~CvrVehicleSet();
 void configure(Reader reader,void* context,const CvrRecipeTable* table,const uint8_t* global_rgb,const uint8_t (*tints)[3],uint8_t tint_mask);
 /* nullptr if the design has no recipe or a part is missing/invalid; failures
  * are remembered so a missing asset is not reread each frame. */
 CvrVehicle* get(int chassis,int weapon,int armor,int reactor,uint32_t abilities);
 void clear();
private:
 CvrVehicleSet(const CvrVehicleSet&)=delete;CvrVehicleSet&operator=(const CvrVehicleSet&)=delete;
 struct Slot{int32_t key[5];CvrVehicle* vehicle;uint32_t used;};
 Slot slots_[6];int32_t failed_[32][5];unsigned failed_count_;uint32_t clock_;
 Reader reader_;void* context_;const CvrRecipeTable* table_;bool global_;uint8_t global_rgb_[768];uint8_t tints_[8][3];uint8_t mask_;
};
}
#endif
