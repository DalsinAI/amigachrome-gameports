/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_CITIZEN_ENERGY_H
#define ALPHA_WORLD_CITIZEN_ENERGY_H
#include "citizens_text.h"
#include "base_energy.h"
#include "original_world.h"
#include "game_logic.h"
namespace alpha {
struct WorldCitizenEnergy {
 Variant variant;
 int32_t faction;
 CitizenEnergyFacts citizens[specialist_definition_count];
};
/* Qualified technology and citizen tables must originate from the same rules
 * input. Citizen edition must match the loaded world (7..10 base,11..12 SMAX).
 * Resolve current ownership through original has_tech semantics: faction0,
 * Disable and future88 are false; None is true only for factions1..7; retain
 * original None/Disable prerequisite asymmetry. Signed bonuses pass unchanged.
 * Complete output is transactional and must be disjoint from both tables and
 * every world allocation. Does not choose/replace specialists, mutate caches,
 * compute income, grant technologies, execute turns, or establish authority.
 */
LogicError world_citizen_energy(const OriginalWorld&,const Rules&,const CitizenDefinitions&,
 int32_t faction,WorldCitizenEnergy&);
}
#endif
