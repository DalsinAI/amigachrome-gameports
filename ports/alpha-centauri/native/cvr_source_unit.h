/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_SOURCE_UNIT_H
#define ALPHA_CVR_SOURCE_UNIT_H
#include "cvr_infantry.h" // Existing borrowed CvrPartInput asset descriptor only.
#include "cvr_scene.h"
namespace alpha {
/* Separate source-scene candidate. Static frame0/basic infantry or speeder only.
 * Two source-selected LOD sets match current viewer sizes; no fitted silhouette.
 * A qualified global palette is required; speeder also requires an explicit tint.
 * Context/variant eligibility is the caller's responsibility. */
class CvrSourceUnit {
public:
 CvrSourceUnit();~CvrSourceUnit();
 bool load(const void*,size_t,const void*,size_t);
 bool load_speeder(const CvrPartInput*,size_t);
 bool set_game_palette(const uint8_t*,size_t,const uint8_t* tint=nullptr,size_t tint_size=0);
 void clear_game_palette();
 bool ready()const{return ready_;}
 bool render_snapshot(unsigned viewer_zoom,unsigned facing,CvrRasterSurface);
 bool draw(uint8_t* rgb,size_t capacity,unsigned width,unsigned height,int tile_x,int tile_y,unsigned viewer_zoom,unsigned facing);
 unsigned cache_generation()const{return generation_;}
private:
 CvrSourceUnit(const CvrSourceUnit&)=delete;CvrSourceUnit&operator=(const CvrSourceUnit&)=delete;
 bool load_parts(const CvrPartInput*,unsigned);
 bool overlaps_owned(const void*,size_t)const;
 void reset_cache();
 CvrModel models_[2][10];uint8_t* cache_[16];unsigned parts_,generation_;bool ready_,palette_ready_;uint8_t rgb_[768],tint_[3],faction_shades_[2][6144];
};
}
#endif
