/* SPDX-License-Identifier: GPL-3.0-or-later. See BASE-COMMERCE.md. */
#ifndef ALPHA_WORLD_BASE_COMMERCE_H
#define ALPHA_WORLD_BASE_COMMERCE_H
#include "base_commerce.h"
#include "world_technology_effects.h"
namespace alpha {
struct WorldCommerceResult {
 int32_t rank,technology_count,total;
 int32_t partner_base[8],import_value[8],export_value[8],fresh_technology_bonus[8];
};
/* Human base-game trade proposal. The vector must contain the authoritative
 * fresh pre-convoy energy_intake for EVERY current base in global base order,
 * and must reside outside all owned-world buffers. Saved energy_intake caches
 * are deliberately never used. Derives actual owned-base counts/ranks, treaty
 * and pact bits, sanctions, current governor, global trade proposals, valid
 * commerce-technology count and fresh per-faction commerce technology effects.
 * Ranking follows the complete original base_rank: intake*512+baseid descending
 * (higher ID wins ties). A treaty must also exist in the partner's status.
 * All failure paths/aliases preserve output and all inputs. No world mutation,
 * alien grid, foreign worker allocation or full-turn claim. */
CommerceError world_base_commerce(const OriginalWorld&,const Rules&,size_t base,
 const int32_t*fresh_allbase_energy,size_t count,WorldCommerceResult&);
}
#endif
