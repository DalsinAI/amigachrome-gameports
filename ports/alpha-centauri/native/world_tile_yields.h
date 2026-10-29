/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_WORLD_TILE_YIELDS_H
#define AC_ALPHA_WORLD_TILE_YIELDS_H
#include "original_world.h"
#include "tile_yields.h"
namespace alpha {
/* These are transient engine-stage facts, not imported authority flags.
 * current_base is required only by the original solar/mirror branch. */
struct WorldTileYieldStage {
 Variant variant;
 bool expansion_enabled,terraform_preview,control_upkeep;
 int32_t current_base,terraform_reduction_before,base_energy_before;
};
/* Pure saved/owned-state query. Requires matched parsed rules/resources.
 * Derives physical facilities, query-faction economics/tech/fungus state,
 * base-owner Aquatic, project ownership, actual map bonus and mirror neighbors.
 * Does not claim original loader fixups or refreshed worker/upkeep state.
 * Input/output buffers must not overlap any world storage or other inputs.
 * Outputs remain unchanged on all failures. */
TileYieldError world_tile_yield_context(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t base,int32_t querying_faction,
 int32_t x,int32_t y,const WorldTileYieldStage&,TileYieldContext& output);
TileYieldError world_tile_yields(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t base,int32_t querying_faction,
 int32_t x,int32_t y,const WorldTileYieldStage&,TileYieldResult& output);
}
#endif
