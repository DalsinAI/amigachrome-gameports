/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_INFANTRY_H
#define ALPHA_CVR_INFANTRY_H
#include "cvr_model.h"
namespace alpha {
/* Bounded static basic-human-infantry presentation profile. It combines original
 * VI body frame0 and Viptr00 reactor frame0 with a common depth surface and each
 * component's own palette. Camera/scale follow recovered flat-unit facts; the
 * tile-foot anchor is new presentation wiring, not original viewport/shadows.
 * Unsupported designs are rejected by the world-view caller.
 * At most100,000 voxels; facing/zoom RGBA caches are lazy and bounded to3MiB. */
class CvrInfantry {
public:
 CvrInfantry();~CvrInfantry();
 bool load(const void* body,size_t body_size,const void* reactor,size_t reactor_size);
 bool draw(uint8_t* rgb,size_t capacity,unsigned width,unsigned height,int center_x,int foot_y,unsigned zoom,unsigned facing);
 bool ready()const{return ready_;}
private:
 CvrInfantry(const CvrInfantry&)=delete;CvrInfantry&operator=(const CvrInfantry&)=delete;
 struct Cache {uint8_t* rgba;};
 CvrModel body_,reactor_;uint8_t* lattice_;bool ready_;Cache cache_[16];
};
}
#endif
