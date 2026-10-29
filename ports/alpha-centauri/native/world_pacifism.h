/* SPDX-License-Identifier: GPL-3.0-or-later
 * Pacifism predicate adapted from Thinker base.cpp, MIT, dabe62f2d1abba03de40f3dd3048494843f6a9af,
 * and privately checked original edition branches. See THINKER-LICENSE.txt.
 */
#ifndef ALPHA_WORLD_PACIFISM_H
#define ALPHA_WORLD_PACIFISM_H
#include "technology_effects.h"
namespace alpha {
struct WorldPacifism {
 Variant variant;int32_t base,count,home_combat,away,air,both;
};
/* Fresh read-only original pacifism aggregate, not base_support/upkeep.
 * Own actors with this home base, unsigned plan<10, nonzero weapon offense.
 * Foreign/neutral territory outside a valid base OR air with plan!=4 counts.
 * Original valid-base predicate is tile base bit AND val2 low nibble<8.
 * Territorial comparison uses original whose_territory(...,nullptr,0) result
 * equivalence: only tile.owner>=1 matching the actor faction returns that
 * faction. Foreign communications flags cannot change that comparison.
 * Clean Reactor, mineral support, pending POLICE and faction immunity do not
 * remove units from this counter. Those are separate source stages.
 * Diagnostics count overlapping away/air reasons, not independent penalties.
 * Qualified same-edition unit definitions; complete output is transactional
 * and disjoint from all inputs and every World allocation. No actor flags,
 * cached counters, accounting, RNG, authority, turns, UI or saves are changed.
 */
LogicError world_pacifism(const OriginalWorld&,const UnitDefinitions&,size_t base,WorldPacifism&);
}
#endif
