/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original radius-ecology behavior independently reconstructed from bounded
 * private original suffix execution. Radius offsets from Thinker path.h,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * No original executable bytes, proprietary tables or assets distributed.
 */
#ifndef ALPHA_WORLD_TERRAFORM_ECOLOGY_H
#define ALPHA_WORLD_TERRAFORM_ECOLOGY_H
#include "original_world.h"
namespace alpha {
enum class TerraformEcologyError { ok,bad_state,bad_buffer };
struct TerraformEcologyResult {
 int32_t raw_damage;
 uint32_t valid_tiles;
 int32_t contributions[21];
};
/* Fresh terraforming damage from actual tile items, physical Tree Farm/Hybrid
 * Forest and the CURRENT owned worked_tiles mask. Counts all in-bounds radius
 * tiles, regardless of visibility or worker availability. Does not allocate
 * workers, force the centre worked, fix stacks, read saved eco_damage or apply
 * mineral/ecology penalties. Per-tile signed halves round toward zero before
 * summation. No authority, transient flags, RNG or turn advancement inferred.
 * Output must be disjoint from all world storage; unchanged on all failures.
 */
TerraformEcologyError world_terraform_ecology(const OriginalWorld&,size_t base,
 TerraformEcologyResult&);
/* Same current physical facts for a proposed first21 worker mask. No forcing
 * centre, imported cache, mutation or assignment authority. Atomic output. */
TerraformEcologyError world_terraform_ecology_for_workers(const OriginalWorld&,
 size_t base,uint32_t worked_tiles,TerraformEcologyResult&);
}
#endif
