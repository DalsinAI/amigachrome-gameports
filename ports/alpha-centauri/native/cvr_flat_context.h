/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_FLAT_CONTEXT_H
#define ALPHA_CVR_FLAT_CONTEXT_H
#include "original_world.h"
namespace alpha {
struct CvrFlatContext {bool supported;uint8_t relative_corner;int32_t corner_height;};
/* Conservative normal RenderScale25 profile only. Sea is flat. Land requires
 * all sixteen corner-neighbours on land, supported un-clamped height bands,
 * no special height-feature lookup and identical resolved relative corners.
 * Rejects edges, slopes and unknown feature/height contexts; no climate-only guess.
 * Returns the common source corner height for current viewer zoom1/2. */
CvrFlatContext cvr_source_flat_context(const OriginalWorld&,int x,int y,unsigned viewer_zoom);
}
#endif
