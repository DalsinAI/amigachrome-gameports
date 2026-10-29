/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_HUMAN_RESEARCH_POOL_H
#define ALPHA_HUMAN_RESEARCH_POOL_H
#include "research_options.h"
namespace alpha {
enum class ResearchPoolPath : uint8_t { directed_human,automatic_selection_required };
struct HumanResearchContext {int32_t faction,ranking,turn;bool human,blind_research;};
struct HumanResearchPool {
 uint16_t ids[technology_count];uint8_t categories[technology_count];
 bool offered[technology_count];size_t count;
 ResearchPoolPath path;
};
/* Normal tech_pick(faction,0,-1,...), not steal/trade/forced-choice modes.
 * Inputs are qualified Rules and original tech_avail results from the same
 * faction/variant. Order is growth,tech,wealth,power then original ID.
 * No RNG, valuation, popup generation, selection or discovery mutation.
 * Automatic paths explicitly return no emulated AI/blind/turn0 choice.
 */
LogicError human_research_pool(const Rules&,const ResearchOptions&,const HumanResearchContext&,HumanResearchPool&);
LogicError world_human_research_pool(const OriginalWorld&,const Rules&,Variant,int32_t faction,HumanResearchPool&);
}
#endif
