/* SPDX-License-Identifier: GPL-3.0-or-later
 * Ranking adapted from OpenSMACX base.cpp, Brendan Casey, and Thinker base.cpp.
 * Original edition commerce behavior checked privately; no owner data here.
 */
#ifndef ALPHA_WORLD_COMMERCE_H
#define ALPHA_WORLD_COMMERCE_H
#include "technology_effects.h"
namespace alpha {
struct CommerceBonuses { Variant variant;int32_t faction[8]; };
struct WorldCommerceResult {
 Variant variant;int32_t base,rank,global_technology_count;
 int32_t partner[7],imports[7],exports[7],total;
 uint32_t qualified_foreign_commerce;
};
/* Actual complete same-file Rules and an explicit coherent current commerce
 * bonus stage are required. The first energy field supplies original ranking
 * and trade valuation, not the fresh intake_2 field. Preserve one-sided treaty
 * admission, outgoing-only pact discount, original nonzero faction base_count,
 * raw governor and proposal pass-count comparison. Sanctions/SMAX alien clear
 * AFTER arithmetic. Every executed signed32 intermediate is checked; no wrap.
 * No World/accounting mutation or active-session/turn/AI/UI authority.
 * Errors/overlap preserve output; every World allocation and input is sealed.
 */
LogicError world_commerce(const OriginalWorld&,const Rules&,const CommerceBonuses&,
 size_t base,WorldCommerceResult&);
/* Explicit fresh technology-effects phase, recomputes all eight faction bonus
 * proposals from actual ownership/rules/pending economy. Never reads saved
 * tech_commerce_bonus. Does not commit those proposals or run a turn. */
LogicError fresh_world_commerce(const OriginalWorld&,const Rules&,size_t base,WorldCommerceResult&);
}
#endif
