/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: the terranx.exe scratch globals used by the base computation, under
 * their Thinker Mod names (src/engine.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see engine/THINKER-LICENSE).
 * They are recomputed for *CurrentBaseID by base_compute() and are not saved.
 * Fix: they are kept outside EngineState (WP2 must not change the core
 * state); engine_bind() does not swap them, so after binding another state
 * call base_compute(1) before reading them.
 */
#include "engine.h"
#include "base_wp2.h"

namespace alpha {
namespace engine {

BaseComputeState base_compute_state = {};

int* BaseUpkeepState = &base_compute_state.base_upkeep_state;
int* BaseUpkeepDrawID = &base_compute_state.base_upkeep_draw_id;
int* BaseUpkeepFlag = &base_compute_state.base_upkeep_flag;
int* ControlUpkeepA = &base_compute_state.control_upkeep_a;
int* BaseGrowthRate = &base_compute_state.base_growth_rate;
int* BaseVehPacifismCount = &base_compute_state.base_veh_pacifism_count;
int* BaseForcesSupported = &base_compute_state.base_forces_supported;
int* BaseForcesMaintCost = &base_compute_state.base_forces_maint_cost;
int* BaseForcesMaintCount = &base_compute_state.base_forces_maint_count;
int* BaseTerraformReduce = &base_compute_state.base_terraform_reduce;
int* BaseTerraformEnergy = &base_compute_state.base_terraform_energy;
int* BaseResourceConvoyTo = base_compute_state.base_resource_convoy_to;
int* BaseResourceConvoyFrom = base_compute_state.base_resource_convoy_from;
int* BaseCommerceImport = base_compute_state.base_commerce_import;
int* BaseCommerceExport = base_compute_state.base_commerce_export;
int* BasePsychTalents = base_compute_state.base_psych_talents;
int* BasePsychNDrones = base_compute_state.base_psych_ndrones;
int* BasePsychSDrones = base_compute_state.base_psych_sdrones;
int* BaseTileFlags = base_compute_state.base_tile_flags;
__attribute__((weak)) int* SkipTechScreenB = &base_compute_state.skip_tech_screen_b;
__attribute__((weak)) int* GameVictoryType = &base_compute_state.game_victory_type;
int* dword_90EA40 = &base_compute_state.prototype_flag_a;
int* dword_90EA3C = &base_compute_state.prototype_flag_b;

void base_compute_state_reset() {
    memset(&base_compute_state, 0, sizeof(base_compute_state));
    base_compute_state.base_upkeep_draw_id = -1;
}

} // namespace engine
} // namespace alpha
