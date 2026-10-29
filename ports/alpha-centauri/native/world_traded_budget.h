/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_TRADED_BUDGET_H
#define ALPHA_WORLD_TRADED_BUDGET_H
#include "world_base_budget.h"
#include "world_base_commerce.h"
namespace alpha {
struct TradedBudgetResult {UpkeepWorldCommerceResult trade;BaseBudgetResult budget;};
/* Connected trade -> energy distribution -> economy income -> maintenance.
 * Stage commerce/grid must be zero: commerce is generated here from the full
 * fresh pre-convoy energy vector and current diplomacy/technology. The selected
 * base's vector entry is verified against a fresh resource calculation; the
 * caller must qualify the other bases' fresh entries in the same stage.
 * No saved intake/commerce is substituted. All public outputs are atomic even
 * if validation, generated trade, allocation or maintenance fails late.
 * This remains a calculation proposal, with the production/growth/riot and
 * persistence prerequisites of world_base_budget; it grants no turn authority.
 */
BudgetError world_traded_budget(const OriginalWorld&,const Rules&,const UnitDefinitions&,
 const BasicRules&,const WorldRules&,const ResourceInfo&,const UpkeepCitizenDefinitions&,
 const FacilityDefinitions&,size_t base,const int32_t*fresh_allbase_energy,size_t count,
 const BaseBudgetStage&,TradedBudgetResult&,uint32_t*projected_support_states,size_t capacity);
}
#endif
