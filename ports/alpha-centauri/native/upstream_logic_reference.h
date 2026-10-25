/* SPDX-License-Identifier: GPL-3.0-or-later
 * Test reference: function bodies from OpenSMACX, Copyright (C) 2013-2021 Brendan Casey.
 * Pin 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c; faction.cpp functions
 * compute_faction_modifiers/social_calc and base.cpp::base_nutrient.
 * Adapters replace Windows BOOL/ZeroMemory, original global addresses,
 * project/facility callbacks and pointer arithmetic across struct members.
 * The branch/arithmetic order is retained. Test only, never production state.
 */
#ifndef AC_ALPHA_UPSTREAM_LOGIC_REFERENCE_H
#define AC_ALPHA_UPSTREAM_LOGIC_REFERENCE_H
#include <stdint.h>
#include <string.h>
namespace source_reference {
using BOOL = bool;
constexpr int MaxSocialCatNum=4, MaxSocialEffectNum=11;
constexpr int SOCIAL_CAT_FUTURE=3, SOCIAL_CAT_VALUES=2, SE_CYBERNETIC=1, SE_THOUGHT_CONTROL=3, SE_POWER=1;
constexpr int RULE_SOCIAL=3, RULE_IMMUNITY=4, RULE_IMPUNITY=5, RULE_PENALTY=6, RULE_ROBUST=10;
constexpr uint32_t SP_NETWORK_BACKBONE=1, SP_CLONING_VATS=2, SP_ASCETIC_VIRTUES=4, SP_LIVING_REFINERY=8;
constexpr int FAC_CHILDREN_CRECHE=1, BSTATE_GOLDEN_AGE_ACTIVE=1, RSC_NUTRIENTS=0;
struct SocialEffect { int32_t values[11]; };
struct SocialCategory { uint32_t choice[4]; };
struct Category { SocialEffect model_effect[4]; };
struct Player { int32_t faction_bonus_count, faction_bonus_id[8], faction_bonus_val1[8], faction_bonus_val2[8]; };
struct PlayerData { SocialEffect soc_effect_base, soc_effect_pending; int32_t nutrient_surplus_total; };
struct Base { uint32_t faction_id_current,state; int32_t nutrient_intake_2,nutrient_consumption,
 nutrient_surplus,nutrients_accumulated,population_size; };
struct Basic { int32_t nutrient_req_citizen; };
static Category SocialCategories[4];
static Player Players[2];
static PlayerData PlayersData[2];
static uint32_t projects;
static bool temple,alien,creche;
static bool has_project(uint32_t flag, [[maybe_unused]] uint32_t faction) { return (projects & flag)!=0; }
static bool has_temple([[maybe_unused]] uint32_t faction) { return temple; }
static bool is_alien_faction([[maybe_unused]] uint32_t faction) { return alien; }
static bool has_fac_built([[maybe_unused]] int facility, [[maybe_unused]] int base) { return creche; }
static int32_t range(int32_t value,int32_t low,int32_t high) { return value<low ? low : value>high ? high : value; }
static void ZeroMemory(void* ptr, size_t n) { memset(ptr,0,n); }
static Base base;
static Base* base_ptr=&base;
static Base** BaseCurrent=&base_ptr;
static int32_t growth,base_id,upkeep;
static int32_t* BaseCurrentGrowthRate=&growth;
static int32_t* BaseIDCurrentSelected=&base_id;
static int32_t* BaseUpkeepStage=&upkeep;
static int32_t BaseCurrentConvoyTo[3],BaseCurrentConvoyFrom[3];
static Basic basic;
static Basic* Rules=&basic;
void compute_faction_modifiers(uint32_t faction_id) {
    ZeroMemory(&PlayersData[faction_id].soc_effect_base, sizeof(SocialEffect));
    int count = Players[faction_id].faction_bonus_count;
    for (int i = 0; i < count; i++) {
        if (Players[faction_id].faction_bonus_id[i] == RULE_SOCIAL) {
            *(PlayersData[faction_id].soc_effect_base.values
                + Players[faction_id].faction_bonus_val1[i])
                += Players[faction_id].faction_bonus_val2[i];
        }
    }
}
void social_calc(SocialCategory *category, SocialEffect *effect, uint32_t faction_id,
    [[maybe_unused]] BOOL toggle, BOOL is_quick_calc) {
    ZeroMemory(effect, sizeof(SocialEffect));
    for (int cat = 0; cat < MaxSocialCatNum; cat++) {
        uint32_t model = category->choice[cat];
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            int effect_val = SocialCategories[cat].model_effect[model].values[eff];
            if (effect_val < 0) {
                if (cat == SOCIAL_CAT_FUTURE) {
                    if (model == SE_CYBERNETIC) {
                        if (has_project(SP_NETWORK_BACKBONE, faction_id)) {
                            effect_val = 0;
                        }
                    } else if (model == SE_THOUGHT_CONTROL) {
                        if (has_project(SP_CLONING_VATS, faction_id)) {
                            effect_val = 0;
                        }
                    }
                } else if (cat == SOCIAL_CAT_VALUES && model == SE_POWER
                    && has_project(SP_CLONING_VATS, faction_id)) {
                    effect_val = 0;
                }
                if (effect_val < 0) {
                    for (int i = 0; i < Players[faction_id].faction_bonus_count; i++) {
                        if (Players[faction_id].faction_bonus_val1[i] == cat
                            && Players[faction_id].faction_bonus_val2[i] == (int)model) {
                            if (Players[faction_id].faction_bonus_id[i] == RULE_IMPUNITY) {
                                effect->values[eff] -= effect_val; // negates neg effects
                            } else if (Players[faction_id].faction_bonus_id[i] == RULE_PENALTY) {
                                effect->values[eff] += effect_val; // doubles neg effects
                            }
                        }
                    }
                }
            }
            effect->values[eff] += effect_val;
        }
    }
    if (!is_quick_calc) {
        if (has_project(SP_ASCETIC_VIRTUES, faction_id)) {
            effect->values[5]++;
        }
        if (has_project(SP_LIVING_REFINERY, faction_id)) {
            effect->values[2] += 2;
        }
        if (has_temple(faction_id)) {
            effect->values[7]++;
            if (is_alien_faction(faction_id)) {
                effect->values[10]++; // bonus documented in conceptsx.txt but not manual
            }
        }
        SocialEffect *effect_calc = effect;
        SocialEffect *effect_base = &PlayersData[faction_id].soc_effect_base;
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            effect_calc->values[eff] += effect_base->values[eff];
        }
        for (int i = 0; i < Players[faction_id].faction_bonus_count; i++) {
            if (Players[faction_id].faction_bonus_id[i] == RULE_IMMUNITY) { // cancels neg effects
                int *eff_faction_mod
                    = (effect->values + Players[faction_id].faction_bonus_val1[i]);
                *eff_faction_mod = range(*eff_faction_mod, 0, 999);
            } else if (Players[faction_id].faction_bonus_id[i] == RULE_ROBUST) {
                int *eff_faction_mod
                    = (effect->values + Players[faction_id].faction_bonus_val1[i]);
                if (*eff_faction_mod < 0) {
                    *eff_faction_mod /= 2; // halves neg effects
                }
            }
        }
    }
}
void base_nutrient() {
    uint32_t faction_id = (*BaseCurrent)->faction_id_current;
    *BaseCurrentGrowthRate = PlayersData[faction_id].soc_effect_pending.values[6];
    if (has_fac_built(FAC_CHILDREN_CRECHE, *BaseIDCurrentSelected)) {
        *BaseCurrentGrowthRate += 2; // +2 on growth scale
    }
    if ((*BaseCurrent)->state & BSTATE_GOLDEN_AGE_ACTIVE) {
        *BaseCurrentGrowthRate += 2;
    }
    (*BaseCurrent)->nutrient_intake_2 += BaseCurrentConvoyTo[RSC_NUTRIENTS];
    (*BaseCurrent)->nutrient_consumption = BaseCurrentConvoyFrom[RSC_NUTRIENTS]
        + (*BaseCurrent)->population_size * Rules->nutrient_req_citizen;
    (*BaseCurrent)->nutrient_surplus = (*BaseCurrent)->nutrient_intake_2
        - (*BaseCurrent)->nutrient_consumption;
    if ((*BaseCurrent)->nutrient_surplus >= 0) {
        if ((*BaseCurrent)->nutrients_accumulated < 0) {
            (*BaseCurrent)->nutrients_accumulated = 0;
        }
    } else if (!((*BaseCurrent)->nutrients_accumulated)) {
        (*BaseCurrent)->nutrients_accumulated = -1;
    }
    if (*BaseUpkeepStage == 1) {
        PlayersData[faction_id].nutrient_surplus_total
            += range((*BaseCurrent)->nutrient_surplus, 0, 99);
    }
}
}
#endif
