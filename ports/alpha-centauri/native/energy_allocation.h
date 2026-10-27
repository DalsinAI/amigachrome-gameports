/* SPDX-License-Identifier: GPL-3.0-or-later
 * Restoration policy adapted from Thinker mod_base_energy/pick_specialist,
 * MIT pin dabe62f2d1abba03de40f3dd3048494843f6a9af. This is not a recovered
 * original base_energy body. See ENERGY-ALLOCATION.md and THINKER-LICENSE.txt.
 */
#ifndef ALPHA_ENERGY_ALLOCATION_H
#define ALPHA_ENERGY_ALLOCATION_H
#include "base_energy.h"
#include "rules_text.h"
namespace alpha {
struct CitizenDefinition {int32_t prerequisite,obsolete,economy,psych,labs;};
struct CitizenDefinitions {CitizenDefinition specialists[7];uint32_t count;};
/* Original CITIZENS parser: ten rows, seven economic specialist rows followed
 * by three ordinary citizen labels. Owns no strings or input pointers. */
Error read_citizens(const uint8_t*,size_t,const Rules&,CitizenDefinitions&);
struct AllocationFacts {
 int32_t fresh_surplus,alloc_labs,alloc_psych,pending_efficiency;
 int32_t population,specialist_count,min_specialist_population;
 uint32_t specialist_types[2];
 uint8_t prerequisite_known[7],obsolete_known[7];
 CitizenDefinitions citizens;
 bool biology_lab,energy_bank,network_node,hologram_theatre;
 bool research_hospital,nanohospital,tree_farm,hybrid_forest,fusion_lab,quantum_lab;
 bool supercollider,theory_everything,space_elevator,longevity_free_market,network_backbone;
 int32_t global_network_nodes,commerce;
};
struct AllocationResult {
 int32_t distributed_energy,economy,psych,labs,unknown_total;
 int32_t coefficient_economy,coefficient_psych,coefficient_labs;
 uint32_t specialist_types[2];
 int32_t specialist_economy,specialist_psych,specialist_labs;
};
/* Fresh pre-allocation surplus is the authoritative input. Slider rounding,
 * specialist upgrades, physical facility/project bonuses and slider imbalance
 * follow the pinned default policy. Biology Lab is fixed original +2; stockpile
 * output belongs to production and is excluded here. Proposes upgraded nibble
 * slots without modifying citizens/world. Any error preserves output. */
EnergyError allocate_energy(const AllocationFacts&,AllocationResult&);
}
#endif
