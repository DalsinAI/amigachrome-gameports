/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_FRESH_ENERGY_H
#define ALPHA_WORLD_FRESH_ENERGY_H
#include "world_convoy_nutrients.h"
#include "upkeep_world_energy.h"
namespace alpha {
struct FreshEnergyStage {StableIntakeStage intake;int32_t commerce,energy_grid;};
struct FreshEnergyResult {WorldConvoyFlows convoys;StableIntakeResult intake;UpkeepWorldEnergyResult energy;EnergyShortfallOrders shortfall;};
/* Actual actor-ordered energy convoys then freshly recomputed stable directed-
 * human worker intake then HQ loss and shortfall proposals. Saved energy fields
 * never stand in for either engine. Commerce/alien-grid generation remains an
 * explicit authoritative prerequisite, not inferred from saved commerce.
 * Worker stage0 restrictions carry through. No world mutation or turn advance. */
StableIntakeError world_fresh_energy(const OriginalWorld&,const Rules&,const BasicRules&,const ResourceInfo&,size_t,const FreshEnergyStage&,FreshEnergyResult&);
}
#endif
