/* SPDX-License-Identifier: GPL-3.0-or-later. See base_psych.h and world_police.h. */
#ifndef ALPHA_WORLD_PSYCH_H
#define ALPHA_WORLD_PSYCH_H
#include "world_police.h"
namespace alpha {
enum class WorldPsychError : uint32_t { ok,bad_state,bad_buffer,overflow,out_of_memory };
struct WorldPsychContext {
 PoliceSnapshotProfile snapshot;
 uint32_t upkeep_stage;
 int32_t current_psych; // newly allocated spending, never inferred from saved accounting
};
struct WorldPsych {
 Variant variant;int32_t base,map_factor,pacifism;uint32_t garrison_count;
 BasePsychResult population;
};
/* Read-only current-world proposal. Requires same-edition qualified Rules,
 * UnitDefinitions and BasicRules. Explicit offline snapshot and stage/spending
 * context do not assert live original network/modal/upkeep globals. Physical
 * facilities, actual project locations/owners, saved pending social effects,
 * first NODRONE grant, ordered validated garrison and fresh home-unit pacifism.
 * Map factor is original quick_root(recomputed tile_count), not stored cache.
 * Invalid stacks/unsupported facts reject rather than repair or substitute.
 * Errors preserve output. All inputs, allocations, wire bytes and RNG remain
 * unchanged. No accounting commit, turn, authority, UI or persistence.
 */
WorldPsychError world_psych(const OriginalWorld&,const Rules&,const UnitDefinitions&,
 const BasicRules&,size_t base,const WorldPsychContext&,WorldPsych&);
}
#endif
