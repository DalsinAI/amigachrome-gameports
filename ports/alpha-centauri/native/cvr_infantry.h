/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_INFANTRY_H
#define ALPHA_CVR_INFANTRY_H
#include "cvr_model.h"
namespace alpha {
struct CvrPartInput {const void* data;size_t size;};
/* Bounded static infantry/speeder presentation profiles, common depth and each
 * component's own palette. Camera/scale follow recovered flat-unit facts; the
 * tile-foot anchor is new wiring, not original viewport/shadows. Faction palette
 * remapping is not implemented. The caller rejects unsupported designs.
 * At most100,000 infantry/160,000 speeder voxels; caches are bounded to3MiB. */
class CvrUnitAssembly {
public:
 CvrUnitAssembly();~CvrUnitAssembly();
 bool load(const void* body,size_t body_size,const void* reactor,size_t reactor_size);
 // Static human speeder parts, in original append order: VRCP00,VGMC,VGMCP,
 // VB,VBP,VSP,vsptb,vsptf,VLIGHTS,VR00. Selected LOD must match at both zooms.
 bool load_speeder(const CvrPartInput* parts,size_t count);
 bool draw(uint8_t* rgb,size_t capacity,unsigned width,unsigned height,int center_x,int foot_y,unsigned zoom,unsigned facing);
 bool ready()const{return ready_;}
private:
 CvrUnitAssembly(const CvrUnitAssembly&)=delete;CvrUnitAssembly&operator=(const CvrUnitAssembly&)=delete;
 struct Cache {uint8_t* rgba;};
 bool load_parts(const CvrPartInput*,unsigned,unsigned,unsigned,float,float);
 CvrModel models_[10];unsigned parts_,limit_;float scale_,root_x_;uint8_t* lattice_;bool ready_;Cache cache_[16];
};
using CvrInfantry=CvrUnitAssembly; // Existing two-part profile API.
}
#endif
