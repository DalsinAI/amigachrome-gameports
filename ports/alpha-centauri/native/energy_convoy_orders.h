/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_ENERGY_CONVOY_ORDERS_H
#define ALPHA_ENERGY_CONVOY_ORDERS_H
#include "original_world.h"
#include "game_logic.h"
namespace alpha {
struct EnergyConvoyOrders {uint32_t visited,changed_orders;uint8_t clear_order[4096];};
/* Complete callback-free original base_energy_costs effect proposal. The signed
 * energy surplus is an explicit current-stage result, not recycled saved
 * accounting or authority. Negative surplus inspects all faction-owned supply
 * plan10 / convoy order3 / energy type2 actors at physical base tiles with a
 * faction nibble0..7. Tile faction need not equal the actor/current base owner;
 * home base and stack links are not consulted. Orders alone become0.
 * Original caller's turn>1/stage2/lifecycle gate belongs to the owning engine.
 * This query does not admit a phase, write World state, deliver events, perform
 * maintenance/production/research/upkeep, or expose a player action/save.
 * Qualified same-edition World required; aliases reject before writes.
 */
LogicError energy_convoy_orders(const OriginalWorld&,Variant,size_t base,
 int32_t current_energy_surplus,EnergyConvoyOrders&);
}
#endif
