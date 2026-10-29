/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PRODUCTION_PREFIX_H
#define ALPHA_WORLD_PRODUCTION_PREFIX_H
#include "technology_effects.h"
#include "production_loans.h"
namespace alpha {
struct ProductionPrefixResult {
 OriginalFactionState faction;
 int32_t credits[8],energy_before_bases;
};
/* Read-only complete original production setup BEFORE the first base:
 * four phase aggregates reset -> actual technology recomputation -> original
 * ordered loans -> player flag32 cleared/energy scratch proposal -> social
 * arrays/three unknown counters/nutrient/labs cleared. Qualified same-profile
 * Rules required. Other faction fields survive; other factions only propose
 * changed credits. Explicit faction/profile are query context, not authority.
 * No World/credit/tech mutation, base upkeep, phase admission, event pump,
 * discovery, AI, player End Turn or save. Output aliases reject before writes.
 */
LogicError world_production_prefix(const OriginalWorld&,const Rules&,Variant profile,
 int32_t faction,ProductionPrefixResult&);
}
#endif
