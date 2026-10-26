/* SPDX-License-Identifier: GPL-3.0-or-later
 * OpenSMACX (C)2013-2021 Brendan Casey, black_market/base_energy_costs,
 * map.cpp::vector_dist/base_who; pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_WORLD_ENERGY_H
#define ALPHA_WORLD_ENERGY_H
#include "base_energy.h"
#include "original_world.h"
namespace alpha {
struct WorldEnergyStage {int32_t fresh_intake,convoy_to,commerce,energy_grid,convoy_from,upkeep_stage;};
struct WorldEnergyResult {EnergyResult accounting;int32_t headquarters_distance,pending_efficiency;bool physical_creche;};
/* Derives nearest own physical HQ (original999sentinel),physical Creche,
 * pending efficiency and hypothetical stats from the owned snapshot. Transient
 * income/convoy/commerce/grid are authoritative explicit current-stage facts.
 * Queued HQ/Creche grant no effect. No saved resource totals are consumed. */
EnergyError world_energy_accounting(const OriginalWorld&,size_t base,const WorldEnergyStage&,WorldEnergyResult&);
struct EnergyShortfallOrders {uint32_t cancel[64];uint32_t count;};
/* Original004F4DC0 proposal: a negative FRESH surplus cancels ALL same-faction
 * plan10/order3/channel2 convoys standing on any physically marked base tile,
 * including foreign bases and convoys whose home is another base. Exterior
 * energy harvesters stay ordered. Only ORDER_NONE(0) is proposed; no mutation,
 * movement refund, worker recompute, UI popup or turn-authority claim. */
EnergyError world_energy_shortfall_orders(const OriginalWorld&,size_t base,int32_t fresh_surplus,EnergyShortfallOrders&);
}
#endif
