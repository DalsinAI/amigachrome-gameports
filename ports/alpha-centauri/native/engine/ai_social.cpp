/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: social engineering choices.
 * Ported from Thinker Mod src/faction.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations from Thinker are marked "Fix:" (behaviour) or "Original:"
 * (replaced executable or Windows code) and listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {
namespace ai {

static int max_social_value(int faction_id, SocialEffect SE_type) {
    auto initial = (CSocialEffect*)&Factions[faction_id].SE_economy_base;
    int value = initial->values[SE_type];
    for (int sf = 0; sf < 4; ++sf) {
        int cur_val = INT_MIN;
        for (int sm = 0; sm < 4; ++sm) {
            if (society_avail(sf, sm, faction_id)) {
                cur_val = max(cur_val, SocialField[sf].soc_effect[sm].values[SE_type]);
            }
        }
        if (cur_val > INT_MIN) {
            value += cur_val;
        }
    }
    return value;
}

/*
Improved social engineering AI choices feature.
*/
static int social_score(int faction_id, int sf, int sm, int pop_boom) {
    assert(sf >= 0 && sf < 4 && sm >= 0 && sm < 4);
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    int def_val = plans[faction_id].defense_modifier;
    int base_val = max(1, f->base_count);
    int base_ratio = min(10, 10 * f->base_count / clamp(*MapAreaSqRoot / 2, 8, 40));
    int morale_mod = (has_project(FAC_COMMAND_NEXUS, faction_id) ? 2 : 0)
        + (has_project(FAC_CYBORG_FACTORY, faction_id) ? 2 : 0);
    int probe_mod = (*CurrentTurn - m->thinker_last_mc_turn < 10 ? def_val + 1 : 0);
    int sc = 0;
    CSocialEffect vals;
    CSocialCategory soc;
    memcpy(&soc, &f->SE_Politics, sizeof(soc));
    auto current = (CSocialCategory*)&f->SE_Politics;

    if (soc.models[sf] == sm) {
        // Evaluate the current active social model.
        memcpy(&vals, &f->SE_economy, sizeof(vals));
    } else {
        // Take the faction base social values and apply all modifiers.
        soc.models[sf] = sm;
        memcpy(&vals, &f->SE_economy_base, sizeof(vals));
        social_calc(&soc, &vals, faction_id, 0, 0);
    }
    if (m->soc_priority_category >= 0 && m->soc_priority_model >= 0) {
        if (sf == m->soc_priority_category) {
            if (sm == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if (sm != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        } else {
            if (current->models[m->soc_priority_category] == m->soc_priority_model) {
                sc += conf.social_ai_bias;
            } else if (current->models[m->soc_priority_category] != SOCIAL_M_FRONTIER) {
                sc -= conf.social_ai_bias;
            }
        }
    }
    // AIs also take into account Social Effect priorities whenever social_ai_bias >= 10
    if (m->soc_priority_effect >= 0 && m->soc_priority_effect < MaxSocialEffectNum) {
        sc += clamp(vals.values[m->soc_priority_effect], -4, 4)
            * clamp(conf.social_ai_bias / 10, 0, 2);
    }
    if (vals.economy >= 2) {
        sc += (vals.economy >= 4 ? 16 : 12);
    }
    if (vals.efficiency < -2) {
        sc -= (vals.efficiency < -3 ? 16 : 8);
    }
    if (vals.support < -3) {
        sc -= 16;
    }
    if (vals.morale >= 1 && vals.morale + morale_mod >= 4) {
        sc += 10;
    }
    if (vals.probe >= 3 && !has_project(FAC_HUNTER_SEEKER_ALGORITHM, faction_id)) {
        sc += 4 * def_val;
    }
    sc += max(2, 2 + 4*f->AI_wealth + 3*f->AI_tech - f->AI_fight)
        * clamp(vals.economy, -3, 5);
    sc += max(2, 2*f->AI_wealth + 2*f->AI_tech - f->AI_fight + base_ratio/2)
        * clamp(vals.efficiency, -4, 6);
    sc += max(2, 3 + 2*f->AI_power + 2*f->AI_fight - base_ratio/4 + def_val/2)
        * clamp(vals.support, -4, 3);
    sc += max(2, def_val + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals.morale, -4, 4);
    bool creche = has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)
        || has_free_facility(FAC_CHILDREN_CRECHE, faction_id);
    bool sphere = has_tech(Facility[FAC_PUNISHMENT_SPHERE].preq_tech, faction_id);
    bool nodrones = has_free_facility(FAC_PUNISHMENT_SPHERE, faction_id);
    bool skipdrones = has_project(FAC_TELEPATHIC_MATRIX, faction_id);

    if (skipdrones && !nodrones) {
        sc += 4*clamp(vals.talent, 0, 5); // consider only extra talents
    } else if (!nodrones) {
        sc += 4*clamp(vals.talent, -5, 5);
        sc += (vals.police >= 0 || def_val > 2 ? 4 : 2)
            * clamp(vals.police, (def_val > 2 ? -10 : -5), 3);
        if (vals.police < -2) {
            sc -= (vals.police < -3 ? 2 : 1) * def_val
                * (has_aircraft(faction_id) ? 2 : 1);
        }
        if (has_project(FAC_LONGEVITY_VACCINE, faction_id) && sf == SOCIAL_C_ECONOMICS) {
            sc += (sm == SOCIAL_M_PLANNED ? 10 : 0);
            sc += (sm == SOCIAL_M_SIMPLE || sm == SOCIAL_M_GREEN ? 5 : 0);
        }
        int drone_score = 3 + (m->rule_drone > 0) - (m->rule_talent > 0) - (sphere ? 1 : 0);
        if (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN
        && un_charter() && vals.police >= 0) {
            sc += 3*drone_score;
        }
        if (!un_charter() && vals.police >= 0) {
            sc += 2*drone_score;
        }
    }
    if (!has_project(FAC_CLONING_VATS, faction_id)) {
        if (pop_boom && vals.growth + (creche ? 2 : 0) >= GrowthPopBoom) {
            sc += 20;
        }
        if (vals.growth < -2) {
            sc -= 5*clamp(6 - def_val, 2, 4);
        }
        sc += (def_val < 3 ? 4 + (def_val < 2) + 2*pop_boom : 3 + pop_boom)
            * clamp(vals.growth, -3, GrowthPopBoom);
    }
    if (plans[faction_id].keep_fungus) {
        sc += 3*clamp(vals.planet, -3, 0); // penalty for reduced fungus yield
    }
    sc += max(2, (f->SE_planet_base > 0 ? 5 : 2) + m->rule_psi/10
        + (has_project(FAC_MANIFOLD_HARMONICS, faction_id) ? 6 : 0)) * clamp(vals.planet, -3, 3);
    sc += max(2, 1 + def_val + probe_mod + 2*f->AI_power + 2*f->AI_fight)
        * clamp(vals.probe, -2, 3);
    sc += (2*clamp(vals.industry, -3, 5) - 8*mineral_factor(faction_id, vals.industry));

    if (!(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        sc += max(2, 3 + 4*f->AI_tech + 2*(f->AI_wealth - f->AI_fight))
            * clamp(vals.research, -5, 5);
    }

    int psy_val = f->social_psych[clamp(vals.talent + 3, 0, 7)][clamp(vals.police + 5, 0, 8)];
    int psy_mod = psy_val / base_val;
    int sup_val = f->social_support[clamp(vals.support + 4, 0, 7)];
    int sup_mod = 4 * sup_val / base_val;
    int eff_val = f->social_effic[clamp(8 - vals.efficiency, 0, 8)];
    int eff_mod = 2 * eff_val / base_val;
    sc += ((skipdrones || nodrones ? 0 : psy_mod) - sup_mod - eff_mod);

    debug("social_values %d %d %8s psy: %d sup: %d eff: %d score: %d %s\n",
        *CurrentTurn, faction_id, m->filename, psy_mod, sup_mod, eff_mod, sc, SocialField[sf].soc_name[sm]);
    return sc;
}

} // namespace ai

int __cdecl mod_social_ai(int faction_id, int a2, int a3, int a4, int a5, CSocialCategory* a6) {
    using namespace ai;
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    AIPlans* p = &plans[faction_id];

    if (is_human(faction_id) || !is_alive(faction_id)) {
        return 0;
    }
    if (!thinker_enabled(faction_id) || !conf.social_ai) {
        design_units(faction_id);
        // Original: social_ai(faction_id, a2, a3, a4, a5, a6), the executable's own
        // social engineering AI, is not ported; such factions keep their choices.
        (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
        return 0;
    }
    if (f->SE_upheaval_cost_paid > 0) {
        social_set(faction_id);
        return 0;
    }
    int def_value = p->defense_modifier;
    int want_pop = 0;
    int pop_total = 0;
    int pop_boom = 0;
    bool has_creche = has_tech(Facility[FAC_CHILDREN_CRECHE].preq_tech, faction_id)
        || has_free_facility(FAC_CHILDREN_CRECHE, faction_id);
    assert(!memcmp(&f->SE_Politics, &f->SE_Politics_pending, 16));

    if (has_project(FAC_CLONING_VATS, faction_id)) {
        // skipped
    } else if (has_creche || f->SE_growth >= GrowthPopBoom-4) {
        for (int i = 0; i < *BaseCount; i++) {
            BASE* b = &Bases[i];
            if (b->faction_id == faction_id) {
                bool creche = has_facility(FAC_CHILDREN_CRECHE, i);
                want_pop += base_growth_goal(i)
                    * ((b->nutrient_surplus >= Rules->nutrient_intake_req_citizen
                    && ((f->SE_growth + (creche ? 2 : 0) + (b->golden_age() ? 2 : 0)
                    >= GrowthPopBoom) || creche)) ? 4 : 1);
                pop_total += b->pop_size;
            }
        }
        if (pop_total > 0 && ((f->SE_growth <= GrowthPopBoom-4 ? 1 : 2) * want_pop) >= pop_total) {
            pop_boom = 1;
            if (max_social_value(faction_id, SE_GROWTH) + (has_creche ? 2 : 0) >= GrowthPopBoom) {
                pop_boom += (want_pop >= 2*pop_total ? 2 : 1);
            }
        }
    }
    debug("social_params %d %d %8s defense: %d creche: %d pop_boom: %d want_pop: %3d pop_total: %3d\n",
        *CurrentTurn, faction_id, m->filename, def_value, has_creche, pop_boom, want_pop, pop_total);
    int score_diff = 1 + (*CurrentTurn + 11*faction_id) % 6;
    int sf = -1;
    int sm2 = -1;
    CSocialCategory soc;
    auto pending = (CSocialCategory*)&f->SE_Politics_pending;
    auto current = (CSocialCategory*)&f->SE_Politics;

    for (int i = 0; i < MaxSocialCatNum; i++) {
        int sm1 = current->models[i];
        int sc1 = social_score(faction_id, i, sm1, pop_boom);
        for (int j = 0; j < MaxSocialModelNum; j++) {
            if (j == sm1 || !society_avail(i, j, faction_id)) {
                continue;
            }
            int sc2 = social_score(faction_id, i, j, pop_boom);
            if (sc2 - sc1 > score_diff) {
                sf = i;
                sm2 = j;
                score_diff = sc2 - sc1;
                memcpy(&soc, &f->SE_Politics, sizeof(soc));
                soc.models[sf] = sm2;
            }
        }
    }
    int cost;
    if (sf >= 0 && f->energy_credits > (cost = social_upheaval(faction_id, &soc))) {
        int sm1 = current->models[sf];
        pending->models[sf] = sm2;
        f->energy_credits -= cost;
        f->SE_upheaval_cost_paid += cost;
        debug("social_change %d %d %8s cost: %d score: %d %s -> %s\n",
            *CurrentTurn, faction_id, m->filename,
            cost, score_diff, SocialField[sf].soc_name[sm1], SocialField[sf].soc_name[sm2]);
    }
    social_set(faction_id);
    design_units(faction_id);
    // Original: consider_designs(faction_id), the executable's own prototype designer,
    // is not ported; design_units above makes the Thinker designs.
    return 0;
}

} // namespace engine
} // namespace alpha
