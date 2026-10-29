/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FRESH_TILE_YIELDS_H
#define ALPHA_WORLD_FRESH_TILE_YIELDS_H
#include "world_tile_yields.h"
#include "technology_effects.h"
namespace alpha {
/* Same explicit runtime-stage boundary as world_tile_yield_context, but all
 * three fungus counters are freshly recomputed from actual known technologies,
 * faction bonuses and pending economy. Saved cache cells are never used by the
 * leaf calculation. This does not mutate caches, ownership, accounting or RNG.
 * Complete technology-effects overflow rejects even if this tile is nonfungus.
 * Output must be disjoint from every complete input/owned-world allocation.
 * Failures preserve outputs. Old snapshot-only APIs remain unchanged. */
TileYieldError world_fresh_tile_yield_context(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t base,int32_t querying_faction,
 int32_t x,int32_t y,const WorldTileYieldStage&,TileYieldContext&);
TileYieldError world_fresh_tile_yields(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t base,int32_t querying_faction,
 int32_t x,int32_t y,const WorldTileYieldStage&,TileYieldResult&);
}
#endif
