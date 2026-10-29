/* SPDX-License-Identifier: GPL-3.0-or-later. Algorithm provenance:
 * OpenSMACX, Brendan Casey,2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c,
 * plus privately checked complete original edition behavior.
 */
#ifndef ALPHA_BASE_ENERGY_H
#define ALPHA_BASE_ENERGY_H
#include "base_psych.h"
#include "energy_allocation.h"
namespace alpha {
struct CitizenEnergyFacts {
 uint32_t prerequisite_available,obsolete_available;
 int32_t economy,psych,labs;
};
enum class AlienGridPolicy : uint32_t {
 reject_uninitialized=0,
 initialize_zero=1 // Explicit deterministic port fix; not original semantics.
};
struct BaseEnergyFacts {
 EnergyAllocationFacts allocation; // Actual current commerce is supplied.
 BasePsychFacts population; // .psych is replaced by the computed spending.
 CitizenEnergyFacts citizens[7];
 int32_t minimum_specialist_size;
 uint32_t specialist_types[2],projects_here;
 int32_t rule_research,network_node_count;
 uint32_t energy_grid_project_owned,alien,expansion_enabled,sanctioned;
 uint32_t partner_aliens,partner_sanctions,qualified_foreign_commerce;
 uint32_t facility_bits64_to69,projects_built_here;
 AlienGridPolicy alien_grid_policy;
 int32_t current_item,previous_item,mineral_surplus;
};
struct BaseEnergyResult {
 EnergyAllocationResult accounting;
 BasePsychResult population;
 uint32_t specialist_types[2];
};
/* Complete explicit-fact energy proposal through psych. Does not compute
 * commerce ranks or resolve technologies, project ownership, garrison or
 * abilities. The caller supplies actual current facts, never default zeros.
 * Input allocations/efficiency/stage/Creche must agree with population facts.
 * SMAX alien/no-qualified-trade paths that read uninitialized source scratch
 * reject unless initialize_zero is explicitly selected. That policy uses
 * facility/project counts from zero and is a documented restoration fix.
 * No World/session authority/mutation, upkeep, turn, UI or save operations.
 * Errors and any input/output overlap leave all inputs/output unchanged.
 */
PsychError base_energy(const BaseEnergyFacts&,BaseEnergyResult&);
}
#endif
