/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 social engineering: effect calculation and model availability, ported
 * from Thinker Mod src/faction.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * social_set and social_select exist only in terranx.exe and are written
 * fresh from the documented original rules (ENGINE-WP5.md).
 * social_upkeep/social_upheaval belong to WP1, social_ai to WP6.
 */
#include "engine.h"
#include "tech.h"
#include "design.h"

namespace alpha {
namespace engine {

void __cdecl compute_faction_modifiers(int faction_id) {
    MFaction* fac = &MFactions[faction_id];
    auto initial = (CSocialEffect*)&Factions[faction_id].SE_economy_base;
    memset(initial, 0, sizeof(CSocialEffect));
    for (int i = 0; i < fac->faction_bonus_count && i < 8; i++) {
        if (fac->faction_bonus_id[i] == RULE_SOCIAL
        && fac->faction_bonus_val1[i] >= 0 && fac->faction_bonus_val1[i] < MaxSocialEffectNum) {
            initial->values[fac->faction_bonus_val1[i]] += fac->faction_bonus_val2[i];
        }
    }
}

/*
Calculate the social engineering effect modifiers for the specified faction.
Original: returns void in Thinker; the exe signature returns int, always 0 here.
*/
int __cdecl social_calc(CSocialCategory* category, CSocialEffect* effect,
int faction_id, int UNUSED(toggle), int is_quick_calc) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    memset(effect, 0, sizeof(CSocialEffect));
    for (int cat = 0; cat < MaxSocialCatNum; cat++) {
        int model = category->models[cat];
        // Fix: ignore out of range models instead of asserting
        if (model < 0 || model >= MaxSocialModelNum) {
            continue;
        }
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            int effect_val = SocialField[cat].soc_effect[model].values[eff];
            if (effect_val < 0) {
                if (cat == SOCIAL_C_FUTURE) {
                    if (model == SOCIAL_M_CYBERNETIC) {
                        if (has_project(FAC_NETWORK_BACKBONE, faction_id)) {
                            effect_val = 0;
                        }
                    } else if (model == SOCIAL_M_THOUGHT_CONTROL) {
                        if (has_project(FAC_CLONING_VATS, faction_id)) {
                            effect_val = 0;
                        }
                    }
                } else if (cat == SOCIAL_C_VALUES && model == SOCIAL_M_POWER
                && has_project(FAC_CLONING_VATS, faction_id)) {
                    effect_val = 0;
                }
                if (effect_val < 0) {
                    for (int i = 0; i < m->faction_bonus_count && i < 8; i++) {
                        if (m->faction_bonus_val1[i] == cat
                        && m->faction_bonus_val2[i] == model) {
                            if (m->faction_bonus_id[i] == RULE_IMPUNITY) {
                                effect->values[eff] -= effect_val; // negates neg effects
                            } else if (m->faction_bonus_id[i] == RULE_PENALTY) {
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
        if (has_project(FAC_ASCETIC_VIRTUES, faction_id)) {
            effect->police++;
        }
        if (has_project(FAC_LIVING_REFINERY, faction_id)) {
            effect->support += 2;
        }
        if (has_temple(faction_id)) {
            effect->planet++;
            if (m->is_alien()) {
                effect->research++;
            }
        }
        auto initial = (CSocialEffect*)&f->SE_economy_base;
        for (int eff = 0; eff < MaxSocialEffectNum; eff++) {
            effect->values[eff] += initial->values[eff];
        }
        for (int i = 0; i < m->faction_bonus_count && i < 8; i++) {
            int bonus_id = m->faction_bonus_id[i];
            int bonus_val = m->faction_bonus_val1[i];
            if ((bonus_id == RULE_IMMUNITY || bonus_id == RULE_ROBUST)
            && bonus_val >= 0 && bonus_val < MaxSocialEffectNum) {
                if (bonus_id == RULE_IMMUNITY) { // cancels neg effects
                    effect->values[bonus_val] = clamp(effect->values[bonus_val], 0, 999);
                } else if (bonus_id == RULE_ROBUST) { // halves neg effects
                    if (effect->values[bonus_val] < 0) {
                        effect->values[bonus_val] /= 2;
                    }
                }
            }
        }
    }
    return 0;
}

/*
Check to see whether the faction can utilize a specific social category and model.
Return Value: Is social category/model available to faction? true/false
*/
int __cdecl society_avail(int soc_category, int soc_model, int faction_id) {
    // Fix: reject out of range arguments
    if (soc_category < 0 || soc_category >= MaxSocialCatNum
    || soc_model < 0 || soc_model >= MaxSocialModelNum) {
        return false;
    }
    if (MFactions[faction_id].soc_opposition_category == soc_category
    && MFactions[faction_id].soc_opposition_model == soc_model) {
        return false;
    }
    return has_tech(SocialField[soc_category].soc_preq_tech[soc_model], faction_id);
}

/*
Fresh: apply the pending social choices. The new effects show at once in the
pending values (SE_*_pending, used by the status display, base psych/yields
of the current turn and the commerce bonus); the current values (SE_*) change
at the next social_upkeep. Bases of the faction are recomputed.
*/
void __cdecl social_set(int faction_id) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum) {
        return;
    }
    Faction* f = &Factions[faction_id];
    auto pending = (CSocialCategory*)&f->SE_Politics_pending;
    social_calc(pending, (CSocialEffect*)&f->SE_economy_pending, faction_id, false, false);
    tech_effects(faction_id);
    int base_id = *CurrentBaseID;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id) {
            set_base(i);
            base_compute(1);
        }
    }
    if (base_id >= 0 && base_id < *BaseCount) {
        set_base(base_id);
    }
}

/*
Fresh: commit new social choices for a faction like the original Social
Engineering screen. The upheaval cost is measured against the choices the
turn started with (SE_Politics..); SE_upheaval_cost_paid records what was
already paid this turn, so changing again charges (or refunds) only the
difference. Models must be available (society_avail) unless unchanged.
Returns the amount charged (negative for a refund) or INT_MIN when refused.
*/
static int social_apply(int faction_id, const CSocialCategory& choices) {
    Faction* f = &Factions[faction_id];
    auto pending = (CSocialCategory*)&f->SE_Politics_pending;
    auto current = (CSocialCategory*)&f->SE_Politics;
    for (int i = 0; i < MaxSocialCatNum; i++) {
        int sm = choices.models[i];
        if (sm < 0 || sm >= MaxSocialModelNum) {
            return INT_MIN;
        }
        if (sm != current->models[i] && sm != pending->models[i]
        && !society_avail(i, sm, faction_id)) {
            return INT_MIN;
        }
    }
    CSocialCategory next = choices;
    int cost = social_upheaval(faction_id, &next);
    int delta = cost - f->SE_upheaval_cost_paid;
    if (delta > 0 && delta > f->energy_credits) {
        return INT_MIN;
    }
    f->energy_credits -= delta;
    f->SE_upheaval_cost_paid = cost;
    memcpy(pending, &next, sizeof(CSocialCategory));
    social_set(faction_id);
    return delta;
}

int social_change(int faction_id, int category, int model) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum
    || category < 0 || category >= MaxSocialCatNum) {
        return -1;
    }
    Faction* f = &Factions[faction_id];
    CSocialCategory choices;
    memcpy(&choices, &f->SE_Politics_pending, sizeof(choices));
    choices.models[category] = model;
    int delta = social_apply(faction_id, choices);
    return delta == INT_MIN ? -1 : delta;
}

static SocialChooser s_social_chooser = nullptr;
static void* s_social_chooser_ctx = nullptr;

SocialChooser set_social_chooser(SocialChooser chooser, void* context) {
    SocialChooser prev = s_social_chooser;
    s_social_chooser = chooser;
    s_social_chooser_ctx = context;
    return prev;
}

/*
Fresh: the human Social Engineering screen. The UI chooser edits a copy of the
pending choices; without one the screen is a popup (SOCIALSELECT) that keeps
the current choices. Non-human factions do nothing (social_ai is WP6).
*/
void __cdecl social_select(int faction_id) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || !is_human(faction_id)) {
        return;
    }
    Faction* f = &Factions[faction_id];
    CSocialCategory choices;
    memcpy(&choices, &f->SE_Politics_pending, sizeof(choices));
    bool changed = false;
    if (s_social_chooser) {
        changed = s_social_chooser(faction_id, &choices, s_social_chooser_ctx);
    } else {
        for (int i = 0; i < MaxSocialCatNum; i++) {
            parse_num(i, choices.models[i]);
        }
        X_pop("SOCIALSELECT", 0);
    }
    if (changed && memcmp(&choices, &f->SE_Politics_pending, sizeof(choices))) {
        if (social_apply(faction_id, choices) == INT_MIN) {
            parse_num(0, social_upheaval(faction_id, &choices) - f->SE_upheaval_cost_paid);
            X_pop("UPHEAVALCOST", 0);
        }
    }
}

} // namespace engine
} // namespace alpha
