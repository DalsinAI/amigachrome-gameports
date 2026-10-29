/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_TILE_YIELDS_H
#define AC_ALPHA_TILE_YIELDS_H
#include "original_map.h"
#include "rules_text.h"
namespace alpha {
enum class TileYieldError { ok,bad_state,overflow,bad_buffer };
/* Fresh base-relative tile calculation. Facilities are physically built bits,
 * not queued facilities; all economy/project/technology/weather facts are
 * explicit current-stage inputs. No saved base intake is an input. */
struct TileYieldContext {
 Variant variant;
 OriginalMapTile tile;
 uint32_t bonus; // actual bonus_at result:0 none,1 nutrient,2 mineral,3 energy
 bool expansion_enabled,terraform_preview;
 int32_t planet_pending,economy_pending,fungus[3],base_count;
 // Aquatic is the physical base owner's rule flag, not necessarily the
 // querying faction's flag. Manifold/governor facts refer to querying faction.
 bool golden_age,manifold_owned,aquatic;
 uint64_t facilities;
 bool allow_three[3],sea_mineral_technology,governor,merchant_exchange,dust_cloud;
 uint32_t solar_flares;
 bool control_upkeep;
 uint32_t echelon_matches; // adjacent mirrors owned by explicit CurrentBase owner
 int32_t mine_nutrient_effect,mine_without_road_limit;
 uint32_t base_event_flags;
 int32_t terraform_reduction_before,base_energy_before;
 // ocean,base,bonus,forest,recycling,improved-land,improved-sea,monolith,borehole
 int32_t resources[9][3];
};
struct TileYieldResult {
 int32_t nutrient,mineral,energy,terraform_reduction,base_energy;
 int32_t reduction_by_resource[3];
 bool base_energy_written;
};
/* Checked original signed32 domain. Failure preserves output. Input/output
 * storage must be disjoint. Base context is required; this does not emulate
 * negative-base-index memory reads. No worker allocation/upkeep/turn mutation. */
TileYieldError calculate_tile_yields(const TileYieldContext&,TileYieldResult&);
/* Original coordinate/map-seed resource placement; no RNG is advanced.
 * Coordinates must be valid parity in a bounded even-width map. */
TileYieldError tile_resource_bonus(const OriginalMapInfo&,const OriginalMapTile&,
 int32_t x,int32_t y,int32_t game_rules,uint32_t& output);
const char* tile_yield_error_name(TileYieldError);
}
#endif
