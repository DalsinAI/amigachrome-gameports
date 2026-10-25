/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_FIXED_TRANSFORM_H
#define ALPHA_CVR_FIXED_TRANSFORM_H
#include <stdint.h>
namespace alpha {
struct CvrFixedBasis {int16_t xy[6];int32_t z[3];};
struct CvrFixedSeed {int32_t x,y;uint32_t z;int32_t local[3];};
// Basis already includes selected geometry units. Preserve original raw coordinate
// order; this API does not infer axes or recover discarded CVR jump boundaries.
bool prepare_cvr_fixed_basis(const float basis[9],CvrFixedBasis&);
// Jump seeds are signed-short + signed-short; preserve the widened sum.
bool prepare_cvr_fixed_group_seed(const float origin[3],const float basis[9],const int32_t seed[3],CvrFixedSeed&);
bool prepare_cvr_fixed_seed(const float origin[3],const float basis[9],const int16_t seed[3],CvrFixedSeed&);
// Position is absolute in this exact group's original geometry coordinates.
void cvr_fixed_position(const CvrFixedBasis&,const CvrFixedSeed&,const int16_t position[3],int32_t& x,int32_t& y,uint32_t& z);
}
#endif
