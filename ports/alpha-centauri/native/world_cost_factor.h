/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_COST_FACTOR_H
#define ALPHA_WORLD_COST_FACTOR_H
#include "cost_factor.h"
#include "original_world.h"
#include "rules_text.h"
namespace alpha {
/* Complete human path: all facts derive from the owned world and BasicRules.
 * Optional base -1 suppresses Creche/GoldenAge effects exactly as the source.
 * A supplied base must belong to faction. AI returns unsupported because the
 * original great_satan/climactic-battle dependency is not integrated here;
 * no caller-supplied threat/cost scalar substitutes for that engine.
 * Returns a proposal only. All world/rule storage must be disjoint from output.
 * Invalid/unsupported/aliased calls preserve output. No world mutation occurs.
 */
CostFactorError world_cost_factor(const OriginalWorld&,const BasicRules&,
 int32_t faction,uint32_t resource,int32_t base,int32_t&output);
CostFactorError world_base_cost_factor(const OriginalWorld&,const BasicRules&,
 size_t base,uint32_t resource,int32_t&output);
}
#endif
