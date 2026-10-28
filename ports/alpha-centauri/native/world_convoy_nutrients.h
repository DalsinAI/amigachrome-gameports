/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_CONVOY_NUTRIENTS_H
#define ALPHA_WORLD_CONVOY_NUTRIENTS_H
#include "world_fresh_nutrients.h"
namespace alpha {
struct WorldConvoyFlows {
 int32_t to[4],from[4],harvesters;
 int32_t terraform_reduction_after,base_energy_after;
};
struct ConvoyNutrientResult {WorldConvoyFlows convoys;FreshNutrientResult fresh;};
/* Read-only original convoy contribution, in actor order. Uses actual plan10,
 * order3, home/faction/coordinates/channel and physical map base markers.
 * Harvests call fresh tile leaves; never saved intake. Channel3 contributes
 * zero harvest/reset scratch, but retains original unit-count transfers.
 * Fungus harvests use complete fresh technology effects without cache writes.
 * Current-base/upkeep/control/scratch are explicit engine-stage facts.
 * No support/pacifism/actor-state/accounting/authority/upkeep/turn mutation.
 * Leaf safe domain is conservative: overflow in an unused tile resource also
 * rejects. All inputs/output must be disjoint; failure preserves output.
 */
StableIntakeError world_convoy_flows(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t,const StableIntakeStage&,
 WorldConvoyFlows&);
/* Derive convoys, compute stable fresh intake, then nutrient proposal using
 * actual flow channel0 and the resulting transient scratch. */
StableIntakeError world_convoy_nutrients(const OriginalWorld&,const Rules&,
 const BasicRules&,const ResourceInfo&,size_t,const StableIntakeStage&,
 ConvoyNutrientResult&);
}
#endif
