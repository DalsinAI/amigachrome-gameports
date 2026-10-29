/* SPDX-License-Identifier: GPL-3.0-or-later
 * TEST ONLY: exact OpenSMACX base.cpp::cost_factor body, Brendan Casey
 * (C)2013-2021; pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Original offset004E4430. Global-name bindings below are test adapters.
 * This does not execute the original game executable.
 */
#ifndef ALPHA_UPSTREAM_COST_FACTOR_REFERENCE_H
#define ALPHA_UPSTREAM_COST_FACTOR_REFERENCE_H
#include "cost_factor.h"
namespace cost_factor_reference {
using BOOL=bool;
static const alpha::CostFactorFacts* facts;
static uint32_t difficulty,map_size;
static uint32_t*DiffLevelCurrent=&difficulty;
static uint32_t*MapSizePlanet=&map_size;
static bool multiplayer;
static bool*IsMultiplayerNet=&multiplayer;
static uint32_t FactionRankings[8];
struct RuleInputs{uint32_t mineral_cost_multi,nutrient_cost_multi;};
static RuleInputs rules;
static RuleInputs*Rules=&rules;
struct Social{int industry,growth;};
struct Player{Social soc_effect_pending;};
static Player PlayersData[8];
struct Base{uint32_t state;};
static Base Bases[1];
constexpr uint32_t RSC_MINERALS=1,FAC_CHILDREN_CRECHE=2,BSTATE_GOLDEN_AGE_ACTIVE=4;
static bool is_human(uint32_t id){return id==1?facts->human:facts->leader_human;}
static bool great_satan(uint32_t,bool){return facts->leader_threat;}
static bool has_fac_built(uint32_t,int){return facts->children_creche;}
static int range(int n,int low,int high){return n<low?low:n>high?high:n;}
#define __cdecl
int __cdecl cost_factor(uint32_t faction_id, uint32_t rsc_type, int base_id) {
    const uint32_t diff_cost_base[] = { 13, 12, 11, 10, 8, 7 };
    uint32_t factor = is_human(faction_id) ? 10 : diff_cost_base[*DiffLevelCurrent]
        - great_satan(FactionRankings[7], false)
        - (!*IsMultiplayerNet && is_human(FactionRankings[7]));
    uint32_t cost_multiplier = rsc_type ? Rules->mineral_cost_multi : Rules->nutrient_cost_multi;
    if (cost_multiplier != 10) {
        factor = (factor * cost_multiplier) / 10;
    }
    if (*MapSizePlanet == 1) {
        factor = (factor * 9) / 10;
    } else if (!*MapSizePlanet) {
        factor = (factor * 8) / 10;
    }
    if (rsc_type) {
        if (rsc_type == RSC_MINERALS) {
            switch (PlayersData[faction_id].soc_effect_pending.industry) {
              case -7:
              case -6:
              case -5:
              case -4:
              case -3:
                return (factor * 13 + 9) / 10;
              case -2:
                return (factor * 6 + 4) / 5;
              case -1:
                return (factor * 11 + 9) / 10;
              case 0:
                return factor;
              case 1:
                return (factor * 9 + 9) / 10;
              case 2:
                return (factor * 4 + 4) / 5;
              case 3:
                return (factor * 7 + 9) / 10;
              case 4:
                return (factor * 3 + 4) / 5;
              default:
                return (factor + 1) / 2;
            }
        } else {
            return factor; // Energy/PSI? Legacy code logic in case cost used these?
        }
    } else { // nutrient
        int growth_factor = PlayersData[faction_id].soc_effect_pending.growth;
        if (base_id >= 0) {
            if (has_fac_built(FAC_CHILDREN_CRECHE, base_id)) {
                growth_factor += 2; // +2 on growth scale
            }
            if (Bases[base_id].state & BSTATE_GOLDEN_AGE_ACTIVE) {
                growth_factor += 2;
            }
        }
        growth_factor = range(growth_factor, -2, 5);
        return (factor * (10 - growth_factor) + 9) / 10;
    }
}

#undef __cdecl
static int evaluate(const alpha::CostFactorFacts&f){
 facts=&f;difficulty=f.difficulty;map_size=f.map_size;multiplayer=f.multiplayer_net;
 FactionRankings[7]=2;rules={f.multiplier,f.multiplier};
 PlayersData[1].soc_effect_pending={f.industry_pending,f.growth_pending};
 Bases[0].state=f.golden_age?4u:0u;
 return cost_factor(1,f.resource,f.base_present?0:-1);
}
}
#endif
