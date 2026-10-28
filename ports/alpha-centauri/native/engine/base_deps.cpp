/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: weak definitions of helpers the base code needs from other Thinker
 * Mod files (game.cpp, faction.cpp, probe.cpp, plan.cpp, path.cpp, build.cpp,
 * veh.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af), copied verbatim.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * They are weak so the package that ports the owning file supplies the strong
 * definition; see base_deps.h. Changes: asserts comparing with terranx.exe
 * routines removed (the executable is not available).
 * Integration: steal_tech, prototype_factor, mod_proto_cost, mod_base_cost,
 * mod_veh_cost, prune_protos and upgrade_prototypes are WP5's (tech.cpp,
 * design.cpp); their weak copies were removed here.
 */
#include "engine.h"
#include "base_wp2.h"
#include "base_deps.h"

namespace alpha {
namespace engine {

static int wp2_diplo_fear_scale = 0;
static int wp2_diplo_can_surrender = 0;
__attribute__((weak)) int* DiploFearScale = &wp2_diplo_fear_scale;
__attribute__((weak)) int* DiploCanSurrender = &wp2_diplo_can_surrender;

/*
Calculate the offset and bitmask for the specified input.
*/
__attribute__((weak)) void __cdecl bitmask(uint32_t input, uint32_t* offset, uint32_t* mask) {
    *offset = input / 8;
    *mask = 1 << (input & 7);
}

__attribute__((weak)) bool un_charter() {
    return ProposalPassCount[PROP_REPEAL_UN_CHARTER]
        <= ProposalPassCount[PROP_REINSTATE_UN_CHARTER];
}

__attribute__((weak)) bool global_trade_pact() {
    return ProposalPassCount[PROP_REPEAL_GLOBAL_TRADE_PACT]
        < ProposalPassCount[PROP_GLOBAL_TRADE_PACT];
}

__attribute__((weak)) bool victory_done() {
    return (*GameState & STATE_GAME_DONE);
}

__attribute__((weak)) bool full_game_turn() {
    return !(*GameState & STATE_GAME_DONE) || (*GameState & STATE_FINAL_SCORE_DONE);
}

__attribute__((weak)) bool voice_of_planet() {
    // Even destroyed Voice of Planet allows building Ascent to Transcendence
    return project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt;
}

__attribute__((weak)) int __cdecl generators(int faction_id, int* pop_size_req) {
    if (pop_size_req) {
        *pop_size_req = 0;
    }
    if (!MFactions[faction_id].is_alien()) {
        return 0;
    }
    int completed = 0;
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id && base->has_fac_built(FAC_SUBSPACE_GENERATOR)) {
            if (base->pop_size >= Rules->base_size_subspace_gen) {
                ++completed;
            } else if (pop_size_req) {
                ++(*pop_size_req);
            }
        }
    }
    return completed;
}

__attribute__((weak)) bool has_wmode(int faction_id, VehWeaponMode mode) {
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].mode == mode && has_tech(Weapon[i].preq_tech, faction_id)) {
            return true;
        }
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        UNIT* u = &Units[i];
        if (veh_avail(i, faction_id, -1) && u->weapon_mode() == mode) {
            return true;
        }
    }
    return false;
}

__attribute__((weak)) int __cdecl spying(int faction_id) {
    return has_treaty(MapWin->cOwner, faction_id, DIPLO_HAVE_INFILTRATOR)
        || has_project(FAC_EMPATH_GUILD, MapWin->cOwner)
        || (MapWin->cOwner == *GovernorFaction && !MFactions[faction_id].is_alien());
}

__attribute__((weak)) int __cdecl council_votes(int faction_id) {
    if (is_alien(faction_id)) {
        return 0;
    }
    int votes = 0;
    for (int i = 0; i < *BaseCount; i++) {
        if (Bases[i].faction_id == faction_id ) {
            votes += Bases[i].pop_size;
        }
    }
    if (has_project(FAC_EMPATH_GUILD, faction_id)) {
        votes += votes / 2; // +50% votes
    }
    if (has_project(FAC_CLINICAL_IMMORTALITY, faction_id)) {
        votes *= 2; // Doubles votes
    }
    int bonus_count = MFactions[faction_id].faction_bonus_count;
    for (int i = 0; i < bonus_count; i++) {
        if (MFactions[faction_id].faction_bonus_id[i] == RULE_VOTES) {
            int votes_bonus = MFactions[faction_id].faction_bonus_val1[i];
            if (votes_bonus >= 0) {
                votes *= votes_bonus; // Peacekeeper bonus
            }
        }
    }
    return votes;
}

__attribute__((weak)) int __cdecl great_beelzebub(int faction_id, int is_aggressive) {
    if (is_human(faction_id) && FactionRankings[7] == faction_id) {
        int bases_threat = (*CurrentTurn + 25) / 50;
        if (bases_threat < 4) {
            bases_threat = 4;
        }
        if (Factions[faction_id].base_count > bases_threat
        && (Factions[faction_id].diff_level > DIFF_SPECIALIST
        || *GameRules & RULES_INTENSE_RIVALRY || is_aggressive)) {
            return true;
        }
    }
    return false;
}

__attribute__((weak)) int __cdecl great_satan(int faction_id, int is_aggressive) {
    if (great_beelzebub(faction_id, is_aggressive)) {
        bool intense_rivalry = (*GameRules & RULES_INTENSE_RIVALRY);
        if (*CurrentTurn <= ((intense_rivalry ? 0
        : (DIFF_TRANSCEND - Factions[faction_id].diff_level) * 50) + 100)) {
            return false;
        }
        if (climactic_battle() && aah_ooga(faction_id, -1) == faction_id) {
            return true;
        }
        int diff_factor;
        int factor;
        if (intense_rivalry) {
            factor = 4;
            diff_factor = DIFF_TRANSCEND;
        } else if (Factions[faction_id].diff_level >= DIFF_LIBRARIAN
        || *GameRules & RULES_VICTORY_CONQUEST || *ObjectiveReqVictory <= 1000) {
            factor = 2;
            diff_factor = DIFF_LIBRARIAN;
        } else {
            factor = 1;
            diff_factor = DIFF_TALENT;
        }
        return (factor * FactionRankingsUnk[FactionRankings[7]]
            >= diff_factor * FactionRankingsUnk[FactionRankings[6]]);
    }
    return false;
}

__attribute__((weak)) int __cdecl aah_ooga(int faction_id, int pact_faction_id) {
    if (!(*GameRules & RULES_VICTORY_DIPLOMATIC)) {
        return 0; // Diplomatic Victory not allowed
    }
    int votes_total = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        votes_total += council_votes(i);
    }
    int value = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != pact_faction_id
        && (pact_faction_id <= 0 || !has_treaty(i, pact_faction_id, DIPLO_PACT)
        || !(*GameRules & RULES_VICTORY_COOPERATIVE))) {
            int proposal_preq = Proposal[PROP_UNITE_SUPREME_LEADER].preq_tech;
            if ((has_tech(proposal_preq, faction_id)
            || (proposal_preq >= 0
            && (has_tech(Tech[proposal_preq].preq_tech1, faction_id)
            || has_tech(Tech[proposal_preq].preq_tech2, faction_id))))
            && council_votes(i) * 2 >= votes_total && (!value || i == faction_id)) {
                value = i;
            }
        }
    }
    return value;
}

__attribute__((weak)) int __cdecl climactic_battle() {
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_human(i) && Factions[i].corner_market_turn > *CurrentTurn) {
            return true;
        }
    }
    // Removed following code since this will always return false.
    // This may have been disabled as a design decision rather than a bug.
    // if (aah_ooga(0, -1)) { return true; }
    assert(!aah_ooga(0, -1));

    if (voice_of_planet()) { // Replace function ascending(0)
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (is_human(i) && (has_tech(Facility[FAC_PSI_GATE].preq_tech, i)
            || has_tech(Facility[FAC_VOICE_OF_PLANET].preq_tech, i))) {
                return true;
            }
        }
    }
    return false;
}

__attribute__((weak)) int __cdecl steal_energy(int base_id) {
    BASE* base = &Bases[base_id];
    int64_t energy = Factions[base->faction_id].energy_credits;
    if (energy > 0) {
        return clamp((energy * conf.steal_energy_rate * Bases[base_id].pop_size)
            / 100 / max(1, Factions[base->faction_id].pop_total + 1), int64_t{0}, energy);
    }
    return 0;
}

__attribute__((weak)) int satellite_count(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    int value;
    switch (item_id) {
        case FAC_SKY_HYDRO_LAB: value = f.satellites_nutrient; break;
        case FAC_ORBITAL_POWER_TRANS: value = f.satellites_energy; break;
        case FAC_NESSUS_MINING_STATION: value = f.satellites_mineral; break;
        case FAC_ORBITAL_DEFENSE_POD: default: value = f.satellites_ODP; break;
    }
    return value;
}

__attribute__((weak)) int satellite_goal(int faction_id, int item_id) {
    Faction& f = Factions[faction_id];
    AIPlans& p = plans[faction_id];
    int goal = p.satellite_goal;
    if (item_id == FAC_ORBITAL_DEFENSE_POD) {
        int nukes = 0;
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (faction_id != i && is_alive(i) && at_war(faction_id, i)) {
                nukes = max(nukes, Factions[i].planet_busters);
            }
        }
        if (p.enemy_odp > 0 || p.enemy_sat > 0 || nukes > 2) {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8 + nukes/2, 2, 12);
        } else {
            goal = clamp(goal/4, 0, 4) + clamp(f.base_count/8, 2, 4);
        }
    } else {
        if (f.base_count <= 5) {
            goal /= 2;
        }
    }
    if (f.base_count <= 10) {
        goal /= 2;
    }
    return clamp(goal, 0, conf.max_satellites);
}

__attribute__((weak)) int defender_count(int x, int y, int veh_skip_id) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()
        && i != veh_skip_id) {
            num += veh->eval_garrison();
        }
    }
    return (num+1)/4;
}

__attribute__((weak)) int garrison_count(int x, int y) {
    int num = 0;
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->x == x && veh->y == y
        && veh->is_garrison_unit()
        && veh->order != ORDER_SENTRY_BOARD
        && veh->at_target()) {
            num++;
        }
    }
    return num;
}

__attribute__((weak)) int consider_staple(int base_id) {
    BASE* b = &Bases[base_id];
    if (can_staple(base_id)
    && (!un_charter() || (*SunspotDuration > 1 && *DiffLevel >= DIFF_LIBRARIAN))
    && base_can_riot(base_id, true)
    && b->nerve_staple_count < 4
    && b->pop_size / 4 >= b->nerve_staple_count
    && b->drone_total + b->specialist_adjust > 0
    && (b->faction_id == b->faction_id_former || !is_alive(b->faction_id_former)
    || want_revenge(b->faction_id, b->faction_id_former))) {
        if (b->drone_riots_active() || b->drone_total + b->specialist_adjust
        > b->talent_total + (b->nutrient_surplus > 0 && b->mineral_surplus > 0)) {
            action_staple(base_id);
        }
    }
    return 0;
}

__attribute__((weak)) int wp2_project_hurry_score(int UNUSED(base_id), FacilityId UNUSED(item_id)) {
    return 0;
}

__attribute__((weak)) void __cdecl mod_world_volcano(int UNUSED(x), int UNUSED(y), int UNUSED(skip_label)) {
    not_implemented("mod_world_volcano");
}

__attribute__((weak)) void __cdecl world_climate() {
    not_implemented("world_climate");
}

} // namespace engine
} // namespace alpha
