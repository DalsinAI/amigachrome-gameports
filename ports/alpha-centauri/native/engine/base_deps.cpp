/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: weak definitions of helpers the base code needs from other Thinker
 * Mod files (game.cpp, faction.cpp, probe.cpp, plan.cpp, path.cpp, build.cpp,
 * veh.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af), copied verbatim.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * They are weak so the package that ports the owning file supplies the strong
 * definition; see base_deps.h. Changes: asserts comparing with terranx.exe
 * routines removed (the executable is not available).
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

__attribute__((weak)) int __cdecl steal_tech(int faction_id, int faction_id_tgt, int is_steal) {
    if (!faction_id || !faction_id_tgt
    || (*MultiplayerActive && is_human(faction_id) && faction_id != *CurrentPlayerFaction)) {
        return 0;
    }
    int tech_id = mod_tech_pick(faction_id, 0, faction_id_tgt, is_steal ? "STEALTECH" : "ACQUIRETECH");
    if (tech_id < 0) {
        tech_id = 9999;
    }
    if (*MultiplayerActive && faction_id == *CurrentPlayerFaction) {
        // Original: message_data(0x244C, 0, faction_id, tech_id, faction_id_tgt, 0);
        // Network play is not supported by this port.
    } else {
        if (faction_id_tgt == *CurrentPlayerFaction && tech_id != 9999) {
            parse_says(0, get_noun(faction_id), -1, -1);
            // Original: say_tech(StrBuffer, tech_id, 0); the tech name is used directly.
            parse_says(1, (tech_id >= 0 && tech_id < MaxTechnologyNum && Tech[tech_id].name)
                ? Tech[tech_id].name : "", -1, -1);
            NetMsg_pop(NetMsg, "STOLETECH", 5000, 0, 0);
        }
        tech_achieved(faction_id, tech_id, faction_id_tgt, 0);
        if (!is_human(faction_id) && tech_id != 9999) {
            mod_bases_reset(-1, faction_id, 0);
        }
    }
    return tech_id == 9999;
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

__attribute__((weak)) int __cdecl prototype_factor(int unit_id) {
    int faction_id = unit_id / MaxProtoFactionNum;
    if (MFactions[faction_id].rule_flags & RFLAG_FREEPROTO
    || Factions[faction_id].diff_level <= DIFF_SPECIALIST) {
        return 0;
    }
    int triad = Units[unit_id].triad();
    switch (triad) {
    case TRIAD_SEA:
        return Rules->extra_cost_prototype_sea;
    case TRIAD_AIR:
        return Rules->extra_cost_prototype_air;
    case TRIAD_LAND:
    default:
        return Rules->extra_cost_prototype_land;
    }
}

__attribute__((weak)) int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
VehArmor armor_id, VehAblFlag ability, VehReactor reactor_id) {
    uint8_t weap_cost = Weapon[weapon_id].cost;
    // PB check: moved to start vs after 1st triad checks in original > no difference in logic
    if (Chassis[chassis_id].missile && Weapon[weapon_id].offense_value >= 99) {
        return weap_cost;
    }
    uint8_t triad = Chassis[chassis_id].triad;
    uint32_t armor_cost = Armor[armor_id].cost;
    uint32_t speed_cost = Chassis[chassis_id].cost;
    int abil_modifier = 0;
    int flags_modifier = 0;
    if (ability) {
        for (int i = 0; i < MaxAbilityNum; i++) {
            if ((1 << i) & ability) {
                if (abil_modifier) {
                    abil_modifier++; // Increased cost with more than one ability
                }
                int factor = Ability[i].cost;
                if (factor > 0) { // 1+ = Straight Cost; 25% increase per unit of cost
                    abil_modifier += factor;
                } else {
                    switch (factor) {
                      case 0: // None
                      default:
                        break;
                        // Increases w/ ratio of weapon to armor: 0, 1, or 2. Rounded DOWN.
                        // Never higher than 2.
                      case -1:
                        // fixed potential crash: will never trigger in vanilla but could with mods
                        if (armor_cost) {
                            abil_modifier += clamp(weap_cost / armor_cost, 0u, 2u);
                        }
                        break;
                      case -2: // Increases w/ weapon value
                        abil_modifier += weap_cost - 1;
                        break;
                      case -3: // Increases w/ armor value
                        abil_modifier += armor_cost - 1;
                        break;
                      case -4: // Increases w/ speed value
                        abil_modifier += speed_cost - 1;
                        break;
                      case -5: // Increases w/ weapon+armor value
                        abil_modifier += weap_cost + armor_cost - 2;
                        break;
                      case -6: // Increases w/ weapon+speed value
                        abil_modifier += weap_cost + speed_cost - 2;
                        break;
                      case -7: // Increases w/ armor+speed value
                        abil_modifier += armor_cost + speed_cost - 2;
                        break;
                    }
                }
                // 010000000000 - Cost increased for land units; Deep Radar
                // Shifted flag check into main ability loop rather than its
                // own loop at 1st triad checks
                if (Ability[i].flags & AFLAG_COST_INC_LAND_UNIT && triad == TRIAD_LAND) {
                    // separate variable keeps logic same (two abilities, both with cost 0,
                    // one with cost increase flag will trigger above "if (abil_modifier)" if
                    // this is directly abil_modifier++)
                    flags_modifier++;
                }
            }
        }
        abil_modifier += flags_modifier; // adding here keeps logic consistent after optimization
    }
    if (triad == TRIAD_SEA) {
        armor_cost /= 2;
        speed_cost += reactor_id;
    } else if (triad == TRIAD_AIR) {
        if (armor_cost > 1) {
            armor_cost *= reactor_id * 2;
        }
        speed_cost += reactor_id * 2;
    } // TRIAD_LAND ability flag check moved into ability loop above
    uint32_t combat_mod = armor_cost / 2 + 1;
    if (combat_mod < weap_cost) { // which ever is greater
        combat_mod = weap_cost;
    }
    int proto_mod;
    // shifted this check to top vs at end > no difference in logic
    if (combat_mod == 1 && armor_cost == 1 && speed_cost == 1 && reactor_id == REC_FISSION) {
        proto_mod = 1;
    } else {
        // (2 << n) == 2^(n + 1) ; (2 << n) / 2 == 2 ^ n;
        // will crash if reactor_id is 0xFF > divide by zero; not putting in error checking
        // since this is unlikely even with modding however noting for future
        proto_mod = ((speed_cost + armor_cost) * combat_mod + ((2 << reactor_id) / 2))
            / (2 << reactor_id);
        if (speed_cost == 1) {
            proto_mod = (proto_mod / 2) + 1;
        }
        if (weap_cost > 1 && Armor[armor_id].cost > 1) {
            proto_mod++;
            // moved inside nested check vs separate triad checks below > no difference in logic
            if (triad == TRIAD_LAND && speed_cost > 1) {
                proto_mod++;
            }
        }
        // excludes sea probes
        if (triad == TRIAD_SEA && Weapon[weapon_id].mode != WMODE_PROBE) {
            proto_mod = (proto_mod + 1) / 2;
        } else if (triad == TRIAD_AIR) {
            proto_mod /= (Weapon[weapon_id].mode > WMODE_MISSILE) ? 2 : 4; // Non-combat : Combat
        }
        int reactor_mod = (reactor_id * 3 + 1) / 2;
        if (proto_mod < reactor_mod) { // which ever is greater
            proto_mod = reactor_mod;
        }
    }
    int cost = (proto_mod * (abil_modifier + 4) + 2) / 4;
//    debug("proto_cost %d %d %d %d %d cost: %d\n",
//        chassis_id, weapon_id, armor_id, ability, reactor_id, cost);
    return cost;
}

__attribute__((weak)) int __cdecl mod_base_cost(int unit_id) {
    return mod_proto_cost(
        (VehChassis)Units[unit_id].chassis_id,
        (VehWeapon)Units[unit_id].weapon_id,
        (VehArmor)Units[unit_id].armor_id,
        ABL_NONE,
        (VehReactor)Units[unit_id].reactor_id);
}

__attribute__((weak)) int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost) {
    assert(unit_id >= 0 && unit_id < MaxProtoNum);
    int cost = Units[unit_id].cost;
    if (base_id >= 0 && unit_id < MaxProtoFactionNum // Fix: added base_id bounds check
    && (Units[unit_id].offense_value() < 0 || unit_id == BSC_SPORE_LAUNCHER)
    && has_fac_built(FAC_BROOD_PIT, base_id)) {
        cost = (cost * 3) / 4; // Decrease the cost of alien units by 25%
    }
    if (Units[unit_id].plan == PLAN_COLONY && base_id >= 0) {
        cost = clamp(cost, 1, 999);
    }
    int proto_cost_first = 0;
    if (unit_id >= MaxProtoFactionNum && !Units[unit_id].is_prototyped()) {
        proto_cost_first = (base_id >= 0 && has_fac_built(FAC_SKUNKWORKS, base_id))
            ? 0 : (prototype_factor(unit_id) * mod_base_cost(unit_id) + 50) / 100; // moved checks up
        cost += proto_cost_first;
    }
    if (has_proto_cost) {
        *has_proto_cost = proto_cost_first != 0;
    }
    return cost;
}

__attribute__((weak)) int wp2_project_hurry_score(int UNUSED(base_id), FacilityId UNUSED(item_id)) {
    return 0;
}

__attribute__((weak)) void __cdecl prune_protos(int UNUSED(faction_id), int UNUSED(unit_id), int UNUSED(flag)) {
    not_implemented("prune_protos");
}

__attribute__((weak)) void __cdecl upgrade_prototypes(int UNUSED(faction_id), int UNUSED(unit_id)) {
    not_implemented("upgrade_prototypes");
}

__attribute__((weak)) void __cdecl mod_world_volcano(int UNUSED(x), int UNUSED(y), int UNUSED(skip_label)) {
    not_implemented("mod_world_volcano");
}

__attribute__((weak)) void __cdecl world_climate() {
    not_implemented("world_climate");
}

} // namespace engine
} // namespace alpha
