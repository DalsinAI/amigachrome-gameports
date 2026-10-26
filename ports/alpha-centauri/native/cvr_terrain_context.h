/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_TERRAIN_CONTEXT_H
#define ALPHA_CVR_TERRAIN_CONTEXT_H
#include "original_world.h"
namespace alpha {
struct CvrTerrainCorners {uint8_t relative[4];int height[4];};
// Normal RenderScale25 height-cache mathematics for an ordinary land tile.
// Each shared corner that meets ocean or a vertical map edge is zero-height,
// relative4. A valid wrapped longitude uses the original one-step wrap.
// Special landmark height tables and elevation7/cache saturation are deliberately unsupported. The result is
// not scene eligibility: the original basic infantry caller can override its
// normal, and terrain topology/visibility/shadows are separate dependencies.
// Output commits only on success; it must not overlap the owned world.
bool cvr_terrain_corners(const OriginalWorld&,int x,int y,unsigned viewer_zoom,CvrTerrainCorners&);
}
#endif
