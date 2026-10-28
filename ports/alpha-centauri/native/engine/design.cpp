/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 unit design: prototype cost, creation, naming, availability and
 * upgrades. Ported from Thinker Mod src/veh.cpp and src/build.cpp
 * (unit_is_better, unit_score, find_proto) at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * propose_proto, name_proto, retire_proto, prune_protos, scan_prototypes,
 * upgrade_prototypes, upgrade_any_prototypes, consider_designs and
 * design_new_veh exist only in terranx.exe and are written fresh here
 * (ENGINE-WP5.md). "Original:" marks replaced Windows/network code and
 * "Fix:" any behaviour change.
 */
#include "engine.h"
#include "tech.h"
#include "design.h"
#include "wp5_deps.h"

#include <ctype.h>

namespace alpha {
namespace engine {

/* Thinker build.cpp score flags */
enum { SC_Defend = 1, SC_EvalCount = 2 };

static char s_design_buf[StrBufLen];

static const char* safe(const char* s) {
    return s ? s : "";
}

static const char* label_or(size_t index, const char* fallback) {
    const char* text = label_get(index);
    return (text && text[0]) ? text : fallback;
}

static bool valid_components(int chs, int wpn, int arm, int rec) {
    return chs >= 0 && chs < MaxChassisNum && wpn >= 0 && wpn < MaxWeaponNum
        && arm >= 0 && arm < MaxArmorNum && rec >= REC_FISSION && rec <= REC_SINGULARITY;
}

int __cdecl transport_val(VehChassis chassis_id, VehAblFlag abls, VehReactor reactor_id) {
    int value = Chassis[chassis_id].cargo;
    if (Chassis[chassis_id].triad == TRIAD_SEA) {
        value *= reactor_id;
    }
    if (abls & ABL_SLOW) {
        value /= 2; // -50%, rounded down
    }
    if (abls & ABL_HEAVY_TRANSPORT) {
        value = (3 * value + 1) / 2; // +50%, rounded up
    }
    return value;
}

/*
Determine the extra percent cost for building a prototype. Includes a check if the faction
has the free prototype flag set or if the player is using one of the easier difficulties.
*/
int __cdecl prototype_factor(int unit_id) {
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

/*
Calculates cost of the prototype based on various factors. Optimized logic flow from
the original without any differences to the final calculation.
*/
int __cdecl mod_proto_cost(VehChassis chassis_id, VehWeapon weapon_id,
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
            if ((1u << i) & (uint32_t)ability) {
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
        // Fix: reactor ids outside 1..4 are clamped to avoid shifts out of range
        int rec = clamp((int)reactor_id, 0, 30);
        proto_mod = ((speed_cost + armor_cost) * combat_mod + ((2u << rec) / 2))
            / (2u << rec);
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
    return cost;
}

int __cdecl proto_cost(int chassis_id, int weapon_id, int armor_id, int abilities, int reactor_id) {
    return mod_proto_cost((VehChassis)chassis_id, (VehWeapon)weapon_id, (VehArmor)armor_id,
        (VehAblFlag)abilities, (VehReactor)reactor_id);
}

/*
Calculates the base cost of the specified unit (without prototype modifiers).
*/
int __cdecl mod_base_cost(int unit_id) {
    return mod_proto_cost(
        (VehChassis)Units[unit_id].chassis_id,
        (VehWeapon)Units[unit_id].weapon_id,
        (VehArmor)Units[unit_id].armor_id,
        ABL_NONE,
        (VehReactor)Units[unit_id].reactor_id);
}

/*
Calculate the specified prototype's mineral row cost to build. Optional output parameter
whether there is an associated 1st time prototype cost (true) or just the base (false).
Note that has_proto_cost is treated as int32_t variable in the game engine.
*/
int __cdecl mod_veh_cost(int unit_id, int base_id, int32_t* has_proto_cost) {
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

/*
Calculate upgrade cost between two prototypes in mineral rows.
*/
int __cdecl mod_upgrade_cost(int faction_id, int new_unit_id, int old_unit_id) {
    UNIT* old_unit = &Units[old_unit_id];
    UNIT* new_unit = &Units[new_unit_id];
    int cost;

    if (conf.modify_upgrade_cost) {
        if (new_unit->is_supply()) {
            cost = max((int)new_unit->cost, 4*max(1, new_unit->cost - old_unit->cost));
        } else {
            cost = max((int)new_unit->cost, 2*max(1, new_unit->cost - old_unit->cost));
        }
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            cost *= 2;
        }
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
    } else {
        int modifier = new_unit->cost;
        if (new_unit_id >= MaxProtoFactionNum && !new_unit->is_prototyped()) {
            modifier = ((new_unit->cost + 1) / 2 + new_unit->cost)
                * ((new_unit->cost + 1) / 2 + new_unit->cost + 1);
        }
        cost = max(0, new_unit->speed() - old_unit->speed())
            + max(0, new_unit->armor_cost() - old_unit->armor_cost())
            + max(0, new_unit->weapon_cost() - old_unit->weapon_cost())
            + modifier;
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            cost /= 2;
        }
    }
    return cost;
}

/* Original say_stats_2: "offense-defense-moves" with psi shown as "?". */
void __cdecl say_stats_2(char* buf, int unit_id) {
    UNIT* u = &Units[unit_id];
    char tmp[32];
    int off = u->offense_value();
    int def = u->defense_value();
    char a[8], d[8];
    if (off < 0) { snprintf(a, sizeof(a), "?"); } else { snprintf(a, sizeof(a), "%d", off); }
    if (def < 0) { snprintf(d, sizeof(d), "?"); } else { snprintf(d, sizeof(d), "%d", def); }
    snprintf(tmp, sizeof(tmp), "%s-%s-%d", a, d, (int)u->speed());
    strncat(buf, tmp, StrBufLen - strnlen(buf, StrBufLen) - 1);
}

static void append(char* buf, const char* s) {
    strncat(buf, s, StrBufLen - strnlen(buf, StrBufLen) - 1);
}

static void say_unit(char* buf, int unit_id) {
    UNIT* u = &Units[unit_id];
    buf[0] = '\0';
    append(buf, u->name);
    append(buf, " (");
    say_stats_2(buf, unit_id);
    if (u->ability_flags) {
        append(buf, " ");
        for (int i = 0; i < MaxAbilityNum; i++) {
            if (has_abil(unit_id, (VehAblFlag)(1u << i))) {
                append(buf, safe(Ability[i].abbreviation));
            }
        }
    }
    append(buf, ")");
}

int __cdecl mod_upgrade_prototype(int faction_id, int new_unit_id, int old_unit_id, int flag) {
    // Fix: reject invalid arguments instead of asserting
    if (faction_id < 0 || faction_id >= MaxPlayerNum || new_unit_id == old_unit_id
    || new_unit_id < 0 || new_unit_id >= MaxProtoNum || old_unit_id < 0 || old_unit_id >= MaxProtoNum
    || new_unit_id / MaxProtoFactionNum != faction_id) {
        return 0;
    }
    Faction* f = &Factions[faction_id];
    UNIT* new_unit = &Units[new_unit_id];
    UNIT* old_unit = &Units[old_unit_id];

    if (!full_game_turn()) {
        return 0;
    }
    // Fix issue with upgrade cost being incorrect for unit counts of more than 255.
    int unit_cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    int unit_count = veh_count(faction_id, old_unit_id);
    int total_cost = unit_cost * unit_count;
    bool prototyped = new_unit_id < MaxProtoFactionNum || new_unit->is_prototyped();

    if (!total_cost && (flag || !is_human(faction_id))) {
        return 0;
    }
    if (!is_human(faction_id)) {
        // Simplify cost threshold for automatic upgrades
        if (total_cost > f->energy_credits/4
        || unit_cost >= 50 + min(50, f->energy_credits/32)) {
            return 0;
        }
        // Reduce non-important upgrades
        if (conf.design_units && (old_unit->plan > PLAN_RECON
        || (conf.ignore_reactor_power && new_unit->offense_value() <= old_unit->offense_value()
        && new_unit->defense_value() <= old_unit->defense_value()))) {
            return 0;
        }
        full_upgrade(faction_id, new_unit_id, old_unit_id);
        return 1;
    }
    if (faction_id != MapWin->cOwner) {
        return 0;
    }
    if (total_cost) {
        say_unit(s_design_buf, old_unit_id);
        parse_says(0, s_design_buf, -1, -1);
        say_unit(s_design_buf, new_unit_id);
        parse_says(1, s_design_buf, -1, -1);
        parse_num(0, total_cost);
        parse_num(1, f->energy_credits);
        parse_num(2, unit_count);

        const char* image_file;
        switch (new_unit->triad()) {
            case TRIAD_SEA: image_file = "navun_sm.pcx"; break;
            case TRIAD_AIR: image_file = (is_alien(faction_id) ? "alair_sm.pcx" : "air_sm.pcx"); break;
            default: image_file = (is_alien(faction_id) ? "alunit_sm.pcx" : "unit_sm.pcx"); break;
        }
        if (total_cost > f->energy_credits) {
            if (flag && !(*GameWarnings & WARN_STOP_PROTOTYPE_COMPLETE)) {
                return 0;
            }
            const char* text_label = (prototyped ? "UPGRADEPROTOCOST" : "UPGRADEPROTOCOST2");
            popp(ScriptFile, text_label, 0, image_file, 0);
            return 0;
        } else {
            const char* text_label = (prototyped ? "UPGRADEPROTO" : "UPGRADEPROTO2");
            if (!*dword_90EA3C) {
                int value = popp(ScriptFile, text_label, 0, image_file, 0);
                if (value <= 0) { // Fix: a cancelled popup (-1) also declines
                    return 0;
                }
                if (value > 1) { // This dialog option is not visible by default
                    *dword_90EA3C = 1;
                }
            }
        }
    }
    // Original: multiplayer sends message_data(0x2411) instead.
    full_upgrade(faction_id, new_unit_id, old_unit_id);
    return 1;
}

static void veh_upgrade(VEH* veh, int new_unit_id, int old_unit_id, int cost) {
    Faction* f = &Factions[veh->faction_id];
    f->energy_credits -= cost;
    f->units_active[new_unit_id]++;
    f->units_active[old_unit_id]--;
    veh->unit_id = new_unit_id;
    int morale_diff = (has_abil(new_unit_id, ABL_TRAINED) != 0)
        - (has_abil(old_unit_id, ABL_TRAINED) != 0);
    veh->morale = clamp(veh->morale + morale_diff, 0, 6);
    debug("upgrade_unit %d %d %2d %2d cost: %d %s / %s\n",
    *CurrentTurn, veh->faction_id, veh->x, veh->y, cost, Units[old_unit_id].name, veh->name());
}

/*
Partial upgrades until energy reserves run out, MP mode not implemented.
*/
void __cdecl part_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    Faction* f = &Factions[faction_id];
    if (!*MultiplayerActive && faction_id > 0
    && old_unit_id >= 0 && new_unit_id != old_unit_id
    && new_unit_id / MaxProtoFactionNum == faction_id) {
        int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
        bool active = false;
        for (int i = 0; i < *VehCount; i++) {
            VEH* veh = &Vehs[i];
            if (veh->faction_id == faction_id && veh->unit_id == old_unit_id
            && !veh->moves_spent && !veh->damage_taken && base_at(veh->x, veh->y) >= 0) {
                if (f->energy_credits < cost + 20) {
                    break;
                }
                veh_upgrade(veh, new_unit_id, old_unit_id, cost);
                active = true;
            }
        }
        if (active) {
            draw_map(1);
        }
    }
}

/*
Full upgrade to new prototype, retire old prototype and update base queues.
Total cost must be checked by the caller function.
*/
void __cdecl full_upgrade(int faction_id, int new_unit_id, int old_unit_id) {
    int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    bool active = false;
    for (int i = 0; i < *VehCount; i++) {
        VEH* veh = &Vehs[i];
        if (veh->faction_id == faction_id && veh->unit_id == old_unit_id) {
            veh_upgrade(veh, new_unit_id, old_unit_id, cost);
            active = true;
        }
    }
    for (int i = *BaseCount - 1; i >= 0; --i) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            for (int j = 0; j < 10 && j <= base->queue_size; j++) {
                if (base->queue_items[j] == old_unit_id) {
                    base->queue_items[j] = new_unit_id;
                }
            }
        }
    }
    if (new_unit_id >= MaxProtoFactionNum && active) {
        Units[new_unit_id].unit_flags |= UNIT_PROTOTYPED;
    }
    if (old_unit_id >= MaxProtoFactionNum) {
        Units[old_unit_id].unit_flags &= ~UNIT_ACTIVE;
    }
    if (is_human(faction_id)) {
        draw_map(1);
    }
    // Original: Console_update_data(MapWin, 0) for the local player.
}

/*
Check to see whether provided faction and base can build a specific prototype.
Includes checks to prevent SMACX specific units from being built in SMAC mode.
*/
int __cdecl veh_avail(int unit_id, int faction_id, int base_id) {
    // Fix: reject out of range arguments instead of asserting
    if (unit_id < 0 || unit_id >= MaxProtoNum || faction_id < 0 || faction_id >= MaxPlayerNum) {
        return false;
    }
    if (!Units[unit_id].is_active()
    || (Units[unit_id].obsolete_factions & (1 << faction_id))) {
        return false;
    }
    if (unit_id < MaxProtoFactionNum) {
        if (!has_tech(Units[unit_id].preq_tech, faction_id)) {
            return false;
        }
    }
    if (Units[unit_id].plan == PLAN_COLONY
    && *GameRules & RULES_SCN_NO_COLONY_PODS) {
        return false;
    }
    if (base_id >= 0 && Units[unit_id].triad() == TRIAD_SEA
    && !is_coast(Bases[base_id].x, Bases[base_id].y, 0)) {
        return false;
    }
    int wpn_id = Units[unit_id].weapon_id;
    uint32_t abls = Units[unit_id].ability_flags;
    if (!*ExpansionEnabled && (Units[unit_id].armor_id > ARM_PSI_DEFENSE
    || wpn_id == WPN_RESONANCE_LASER || wpn_id == WPN_RESONANCE_BOLT
    || wpn_id == WPN_STRING_DISRUPTOR || wpn_id == WPN_TECTONIC_PAYLOAD
    || wpn_id == WPN_FUNGAL_PAYLOAD
    || abls & ABL_SOPORIFIC_GAS || abls & ABL_DISSOCIATIVE_WAVE
    || abls & ABL_MARINE_DETACHMENT || abls & ABL_FUEL_NANOCELLS
    || abls & ABL_ALGO_ENHANCEMENT
    || unit_id == BSC_SEALURK || unit_id == BSC_SPORE_LAUNCHER
    || unit_id == BSC_BATTLE_OGRE_MK1 || unit_id == BSC_BATTLE_OGRE_MK2
    || unit_id == BSC_BATTLE_OGRE_MK3 || unit_id == BSC_FUNGAL_TOWER
    || unit_id == BSC_UNITY_MINING_LASER)) {
        return false;
    }
    if (unit_id < MaxProtoFactionNum) {
        return true;
    }
    return (unit_id / MaxProtoFactionNum) == faction_id;
}

int __cdecl create_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor rec, VehPlan ai_plan) {
    char name[256];
    mod_name_proto(name, -1, faction_id, chs, wpn, arm, abls, rec);
    int unit_id = propose_proto(faction_id, chs, wpn, arm, abls, rec, ai_plan, (strlen(name) ? name : NULL));
    debug("create_proto  %d %d chs: %d rec: %d wpn: %2d arm: %2d pln: %2d %08X %d %s\n",
        *CurrentTurn, faction_id, chs, rec, wpn, arm, ai_plan, abls, unit_id, name);
    return unit_id;
}

/*
Propose a new prototype to be added for the faction. However this may be skipped
if a similar prototype already exists or other exclude conditions apply on the parameters.
*/
int __cdecl mod_propose_proto(int faction_id, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor rec, VehPlan ai_plan, char* name) {
    if (!valid_components(chs, wpn, arm, rec)) { // Fix: reject invalid components
        return -1;
    }
    int triad = Chassis[chs].triad;
    int arm_v = Armor[arm].defense_value;
    int wpn_v = Weapon[wpn].offense_value;
    if (conf.design_units) {
        if (triad == TRIAD_SEA && wpn_v > 0 && arm_v > 3 && arm_v > wpn_v + 1
        && !(abls & ~(ABL_SLOW|ABL_DEEP_RADAR|ABL_TRAINED))) {
            return -1; // skipped
        }
    }
    return propose_proto(faction_id, chs, wpn, arm, abls, rec, ai_plan, name);
}

/* Component prerequisite of the highest tech level (fresh, see propose_proto). */
static int design_preq_tech(int chs, int wpn, int arm, uint32_t abls, int rec) {
    int techs[4 + MaxAbilityNum];
    int n = 0;
    techs[n++] = Chassis[chs].preq_tech;
    techs[n++] = Weapon[wpn].preq_tech;
    techs[n++] = Armor[arm].preq_tech;
    techs[n++] = Reactor[rec - 1].preq_tech;
    for (int i = 0; i < MaxAbilityNum; i++) {
        if (abls & (1u << i)) {
            techs[n++] = Ability[i].preq_tech;
        }
    }
    int best = TECH_None;
    int best_lvl = -1;
    for (int i = 0; i < n; i++) {
        int t = techs[i];
        if (t >= 0 && t < MaxTechnologyNum) {
            int lvl = tech_level(t, 0);
            if (lvl > best_lvl) {
                best_lvl = lvl;
                best = t;
            }
        }
    }
    return best;
}

static bool proto_known(int unit_id, int faction_id) {
    UNIT* u = &Units[unit_id];
    if (!(u->unit_flags & UNIT_ACTIVE)) {
        return false;
    }
    if (unit_id < MaxProtoFactionNum) {
        return has_tech(u->preq_tech, faction_id) != 0;
    }
    return unit_id / MaxProtoFactionNum == faction_id;
}

static bool proto_in_queue(int unit_id, int faction_id) {
    for (int i = 0; i < *BaseCount; i++) {
        BASE* b = &Bases[i];
        if (b->faction_id == faction_id) {
            for (int j = 0; j < 10 && j <= b->queue_size; j++) {
                if (b->queue_items[j] == unit_id) {
                    return true;
                }
            }
        }
    }
    return false;
}

int free_proto_slots(int faction_id) {
    int n = 0;
    if (faction_id < 0 || faction_id >= MaxPlayerNum) {
        return 0;
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        if (!(Units[faction_id * MaxProtoFactionNum + i].unit_flags & UNIT_ACTIVE)) {
            n++;
        }
    }
    return n;
}

/*
Fresh implementation of the original propose_proto.
1. An active design with the same components that the faction can already use
   (a basic unit with its prerequisite, or an own prototype) is returned
   instead of a new one; an obsolete mark of the faction on it is cleared and
   PLAN_PROTOTYPE_DONE marks an own prototype as built.
2. Otherwise the first inactive slot of the faction (faction*64 .. +63) is
   used; when none is free, an own obsolete prototype without units or queue
   entries is deleted first. No slot: -1.
3. The slot is filled by make_proto, named (given name, else name_proto),
   gets the most advanced component technology as preq_tech, and the plan
   (ai_plan >= 0 replaces the derived plan; PLAN_PROTOTYPE_DONE marks it
   prototyped). New designs carry UNIT_UNK_80 so the tech screen can offer to
   show them.
*/
int __cdecl propose_proto(int faction_id, int chassis_id, int weapon_id, int armor_id, int abilities,
int reactor_id, int plan, char* name) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum
    || !valid_components(chassis_id, weapon_id, armor_id, reactor_id)) {
        return -1;
    }
    const uint32_t abls = (uint32_t)abilities;
    for (int id = 0; id < MaxProtoNum; id++) {
        if (id >= MaxProtoFactionNum && id / MaxProtoFactionNum != faction_id) {
            continue;
        }
        UNIT* u = &Units[id];
        if (!(u->unit_flags & UNIT_ACTIVE)) {
            continue;
        }
        if (id < MaxProtoFactionNum && !has_tech(u->preq_tech, faction_id)) {
            continue;
        }
        if (u->chassis_id == chassis_id && u->weapon_id == weapon_id && u->armor_id == armor_id
        && u->ability_flags == abls && u->reactor_id == reactor_id) {
            u->obsolete_factions &= ~(1 << faction_id);
            if (plan == PLAN_PROTOTYPE_DONE && id >= MaxProtoFactionNum) {
                u->unit_flags |= UNIT_PROTOTYPED;
            }
            return id;
        }
    }
    int unit_id = -1;
    for (int i = 0; i < MaxProtoFactionNum && unit_id < 0; i++) {
        int id = faction_id * MaxProtoFactionNum + i;
        if (!(Units[id].unit_flags & UNIT_ACTIVE)) {
            unit_id = id;
        }
    }
    if (unit_id < 0) {
        for (int i = 0; i < MaxProtoFactionNum && unit_id < 0; i++) {
            int id = faction_id * MaxProtoFactionNum + i;
            if ((Units[id].obsolete_factions & (1 << faction_id))
            && !veh_count(faction_id, id) && !proto_in_queue(id, faction_id)) {
                Units[id].unit_flags &= ~UNIT_ACTIVE;
                unit_id = id;
            }
        }
    }
    if (unit_id < 0) {
        return -1;
    }
    UNIT* u = &Units[unit_id];
    memset(u, 0, sizeof(UNIT));
    Factions[faction_id].units_active[unit_id] = 0;
    Factions[faction_id].units_queue[unit_id] = 0;
    Factions[faction_id].units_lost[unit_id] = 0;
    make_proto(unit_id, chassis_id, weapon_id, armor_id, abilities, reactor_id);
    if (name && name[0]) {
        strcpy_n(u->name, MaxProtoNameLen, name);
    } else {
        name_proto(u->name, unit_id, faction_id, chassis_id, weapon_id, armor_id, abilities, reactor_id);
    }
    u->preq_tech = (int16_t)design_preq_tech(chassis_id, weapon_id, armor_id, abls, reactor_id);
    if (plan >= 0) {
        u->plan = (int8_t)plan;
    } else if (plan == PLAN_PROTOTYPE_DONE) {
        u->unit_flags |= UNIT_PROTOTYPED;
    }
    u->unit_flags |= UNIT_UNK_80;
    debug("propose_proto %d %d chs: %d rec: %d wpn: %2d arm: %2d pln: %2d %08X -> %d %s\n",
        *CurrentTurn, faction_id, chassis_id, reactor_id, weapon_id, armor_id, plan, abls, unit_id, u->name);
    return unit_id;
}

/*
Create a new prototype. Sets initial values for everything except name and preq_tech.
*/
void __cdecl mod_make_proto(int unit_id, VehChassis chassis_id,
VehWeapon weapon_id, VehArmor armor_id, VehAblFlag abls, VehReactor reactor_id) {
    if (unit_id < 0 || unit_id >= MaxProtoNum
    || !valid_components(chassis_id, weapon_id, armor_id, reactor_id)) { // Fix: bounds check
        return;
    }
    UNIT* unit = &Units[unit_id];
    int faction_id = unit_id / MaxProtoFactionNum;
    int option = 0;
    if (unit_id >= MaxProtoFactionNum) {
        bool cond1 = false;
        bool cond2 = false;
        bool cond3 = false;
        int unit_id_loop = 0;
        for (int i = 0; i < 2*MaxProtoFactionNum; i++) {
            unit_id_loop = i % MaxProtoFactionNum;
            if (i >= MaxProtoFactionNum) {
                unit_id_loop += unit_id - unit_id % MaxProtoFactionNum;
            }
            int flags_check = Units[unit_id_loop].unit_flags;
            if (flags_check & UNIT_ACTIVE) {
                if ((unit_id_loop < MaxProtoFactionNum
                && has_tech(Units[unit_id_loop].preq_tech, faction_id))
                || (unit_id_loop >= MaxProtoFactionNum
                && (flags_check & UNIT_PROTOTYPED))) {
                    int loop_weapon_id = Units[unit_id_loop].weapon_id;
                    if (loop_weapon_id == weapon_id) {
                        cond1 = true;
                    }
                    int loop_armor_id = Units[unit_id_loop].armor_id;
                    if (loop_armor_id == armor_id) {
                        cond2 = true;
                    }
                    int loop_chassis_id = Units[unit_id_loop].chassis_id;
                    if (loop_chassis_id == chassis_id) {
                        cond3 = true;
                    }
                    int off_rating = Weapon[weapon_id].offense_value;
                    if (off_rating > 0 && Weapon[loop_weapon_id].offense_value >= off_rating) {
                        cond1 = true;
                    }
                    int def_rating = Armor[armor_id].defense_value;
                    if (def_rating > 0 && Armor[loop_armor_id].defense_value >= def_rating) {
                        cond2 = true;
                    }
                    if (Chassis[chassis_id].triad == Chassis[loop_chassis_id].triad
                    && Chassis[loop_chassis_id].speed >= Chassis[chassis_id].speed) {
                        cond3 = true;
                    }
                }
            }
        }
        if (cond1 && cond2 && cond3) {
            option = (unit_id_loop >= MaxProtoFactionNum
                && Units[unit_id_loop].unit_flags & UNIT_UNK_10) ? 3 : 1;
        }
    }
    unit->chassis_id = (uint8_t)chassis_id;
    unit->weapon_id = (uint8_t)weapon_id;
    unit->armor_id = (uint8_t)armor_id;
    unit->ability_flags = abls;
    unit->reactor_id = (uint8_t)reactor_id;
    unit->cost = (uint8_t)mod_proto_cost(chassis_id, weapon_id, armor_id, abls, reactor_id);
    unit->carry_capacity = (Weapon[weapon_id].mode == WMODE_TRANSPORT)
        ? (uint8_t)transport_val(chassis_id, abls, reactor_id) : 0;

    if (Chassis[chassis_id].missile) {
        if (Weapon[weapon_id].offense_value < 99) { // non-PB missiles
            if (weapon_id == WPN_TECTONIC_PAYLOAD) {
                unit->plan = PLAN_TECTONIC_MISSILE;
                unit->group_id = 22;
            } else if (weapon_id == WPN_FUNGAL_PAYLOAD) {
                unit->plan = PLAN_FUNGAL_MISSILE;
                unit->group_id = 23;
            } else { // Conventional Payload
                unit->plan = PLAN_OFFENSE;
                unit->group_id = 13;
            }
        } else {
            unit->plan = PLAN_PLANET_BUSTER;
            unit->group_id = 14;
        }
    } else if (Weapon[weapon_id].mode >= WMODE_TRANSPORT) { // non-combat
        unit->plan = Weapon[weapon_id].mode;
        unit->group_id = Weapon[weapon_id].mode + 32;
    } else if (Chassis[chassis_id].triad == TRIAD_SEA) { // combat sea
        unit->plan = PLAN_NAVAL_SUPERIORITY;
        unit->group_id = 6; // same value as plan
    } else if (Chassis[chassis_id].triad == TRIAD_AIR) { // combat air
        if (has_abil(unit_id, ABL_AIR_SUPERIORITY)) {
            unit->plan = PLAN_AIR_SUPERIORITY;
            unit->group_id = 9;
        } else {
            unit->plan = PLAN_OFFENSE;
            unit->group_id = 8;
        }
    } else { // TRIAD_LAND combat unit
        unit->plan = PLAN_OFFENSE;
        if (Armor[armor_id].defense_value > 1) {
            unit->plan = PLAN_DEFENSE;
        }
        if (Weapon[weapon_id].offense_value >= Armor[armor_id].defense_value
        && unit->plan == PLAN_DEFENSE) {
            unit->plan = PLAN_COMBAT;
        }
        if (Chassis[chassis_id].speed > 1
        && Weapon[weapon_id].offense_value > Armor[armor_id].defense_value) {
            unit->plan = PLAN_OFFENSE;
        }
        if (abls & (ABL_ARTILLERY | ABL_DROP_POD | ABL_AMPHIBIOUS)) {
            unit->plan = PLAN_OFFENSE;
        }
        unit->group_id = 3;
        if (Armor[armor_id].defense_value > Weapon[weapon_id].offense_value) {
            unit->group_id = 2;
        }
        if (Weapon[weapon_id].offense_value > Armor[armor_id].defense_value * 2) {
            unit->group_id = 1;
        }
        if (Chassis[chassis_id].speed == 2) {
            unit->group_id = 4;
        } else if (Chassis[chassis_id].speed >= 3) {
            unit->group_id = 5;
        }
        if (Weapon[weapon_id].offense_value == 1 && Armor[armor_id].defense_value == 1) {
            if (Chassis[chassis_id].speed > 1) {
                unit->plan = PLAN_RECON;
            }
            if (has_abil(unit_id, ABL_POLICE_2X)) {
                unit->plan = PLAN_DEFENSE;
            }
            unit->group_id = 10;
        }
    }
    unit->icon_offset = -1;
    unit->obsolete_factions = 0;
    unit->combat_factions = (unit_id >= MaxProtoFactionNum) ? (1 << faction_id) : -1;
    unit->unit_flags = UNIT_ACTIVE;
    if (option) {
        if (option & 2) {
            unit->unit_flags = (UNIT_UNK_100|UNIT_UNK_10|UNIT_PROTOTYPED|UNIT_ACTIVE);
        } else {
            unit->unit_flags = (UNIT_UNK_100|UNIT_PROTOTYPED|UNIT_ACTIVE);
        }
    }
}

void __cdecl make_proto(int unit_id, int chassis_id, int weapon_id, int armor_id, int ability, int reactor_id) {
    mod_make_proto(unit_id, (VehChassis)chassis_id, (VehWeapon)weapon_id, (VehArmor)armor_id,
        (VehAblFlag)ability, (VehReactor)reactor_id);
}

/* ---- Naming (Thinker veh.cpp parse_* helpers and mod_name_proto) ---- */

static void parse_abl_name(char* buf, const char* name, bool reduce) {
    name = safe(name);
    const int len = min((int)strlen(name), MaxProtoNameLen);
    // Skip empty abbreviation names (Deep Radar)
    if (!len) {
        return;
    }
    for (int i = 0; i < len; i++) {
        if (i >= 3 && reduce && (len >= 8 || !isalpha((unsigned char)name[i]))) {
            strncat(buf, name, i+1);
            if (!isspace((unsigned char)name[i]) && name[i] != '-') {
                strncat(buf, " ", 2);
            }
            return;
        }
    }
    strncat(buf, name, MaxProtoNameLen);
    if (!isspace((unsigned char)name[len-1]) && name[len-1] != '-') {
        strncat(buf, " ", 2);
    }
}

static void parse_wpn_name(char* buf, VehWeapon wpn, bool reduce) {
    const char* name = safe(Weapon[wpn].name_short);
    const int len = min((int)strlen(name), MaxProtoNameLen);
    if (reduce) {
        for (int i = 0; i < len; i++) {
            if (isspace((unsigned char)name[i])) {
                strncat(buf, name, i+1);
                return;
            }
        }
    }
    if (reduce && strlen(name) >= 10) {
        strncat(buf, name, 8);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

static void parse_arm_name(char* buf, VehArmor arm, bool reduce) {
    const char* name = safe(Armor[arm].name_short);
    if (reduce && strlen(name) >= 8) {
        strncat(buf, name, 4);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

static void parse_chs_name(char* buf, const char* name1, const char* name2) {
    name1 = safe(name1);
    name2 = safe(name2);
    // Convert Speeder to Rover (short form)
    if (strlen(name1) >= 7 && strlen(name1) > strlen(name2) && strlen(name2)) {
        strncat(buf, name2, MaxProtoNameLen);
        strncat(buf, " ", 2);
        return;
    }
    strncat(buf, name1, MaxProtoNameLen);
    strncat(buf, " ", 2);
}

static void parse_chs_name(char* buf, const char* name) {
    strncat(buf, safe(name), MaxProtoNameLen);
    strncat(buf, " ", 2);
}

/*
Thinker's naming rules (mod_name_proto body). The original exe name_proto is
not available; name_proto uses these rules for every design (ENGINE-WP5.md).
Missiles and units without a role use "<weapon> <chassis>".
*/
static void build_proto_name(char* name, VehChassis chs, VehWeapon wpn, VehArmor arm,
VehAblFlag abls, VehReactor UNUSED(rec)) {
    char buf[256] = {};
    bool noncombat = Weapon[wpn].mode == WMODE_SUPPLY
        || Weapon[wpn].mode == WMODE_PROBE
        || Weapon[wpn].mode == WMODE_TERRAFORM
        || Weapon[wpn].mode == WMODE_TRANSPORT
        || Weapon[wpn].mode == WMODE_COLONY;
    bool combat = Weapon[wpn].offense_value != 0
        && Armor[arm].defense_value != 0;
    bool psi_arm = Armor[arm].defense_value < 0;
    bool psi_wpn = Weapon[wpn].offense_value < 0;
    int wpn_v = (psi_wpn ? 2 : Weapon[wpn].offense_value);
    int arm_v = (psi_arm ? 2 : Armor[arm].defense_value);
    int triad = Chassis[chs].triad;
    int spd_v = Chassis[chs].speed;
    bool arty = abls & ABL_ARTILLERY && triad == TRIAD_LAND;
    bool marine = abls & ABL_AMPHIBIOUS && triad == TRIAD_LAND;
    bool intercept = abls & ABL_AIR_SUPERIORITY && triad == TRIAD_AIR;
    bool garrison = combat && !arty && !marine && wpn_v < arm_v && spd_v < 2;
    uint32_t prefix_abls = abls & ~(ABL_ARTILLERY|ABL_AMPHIBIOUS|ABL_CARRIER|ABL_NERVE_GAS
        | (intercept ? ABL_AIR_SUPERIORITY : 0)); // SAM for ground units
    // Battleship names are used only for long range artillery
    bool sea_arty = abls & ABL_ARTILLERY && triad == TRIAD_SEA && conf.long_range_artillery > 0;
    bool lrg_names = (triad != TRIAD_SEA || conf.long_range_artillery < 1);

    if ((!combat && !noncombat) || Chassis[chs].missile) {
        // Fresh: replaces the exe name_proto for these designs.
        parse_wpn_name(buf, wpn, false);
        parse_chs_name(buf, Chassis[chs].offsv1_name);
        strtrail(buf);
        strcpy_n(name, MaxProtoNameLen, buf);
        return;
    }
    for (int i = 0; i < 2; i++) {
        buf[0] = '\0';
        if (abls & ABL_NERVE_GAS) {
            parse_abl_name(buf, Ability[ABL_ID_NERVE_GAS].abbreviation, i > 0);
        }
        for (int j = 0; j < MaxAbilityNum; j++) {
            if (prefix_abls & (1u << j)) {
                parse_abl_name(buf, Ability[j].abbreviation, i > 0);
            }
        }
        if (arm_v != 1 && !(combat && !psi_arm && wpn_v >= (i > 0 ? 2 : 4)*arm_v)) {
            parse_arm_name(buf, arm, i > 0 || prefix_abls || spd_v > 1 || wpn_v >= arm_v
                || (noncombat && strlen(safe(Weapon[wpn].name_short)) >= 10));
        }
        if (combat) {
            if (!garrison) {
                if (triad == TRIAD_LAND && wpn_v == 1) {
                    strncat(buf, label_or(146, "Scout"), MaxProtoNameLen);
                    strncat(buf, " ", 2);
                } else if (triad == TRIAD_LAND && wpn_v == 2 && !psi_wpn && spd_v > 1) {
                    strncat(buf, label_or(182, "Recon"), MaxProtoNameLen);
                    strncat(buf, " ", 2);
                } else if (triad == TRIAD_LAND || wpn_v != 1 || arm_v == 1) {
                    parse_wpn_name(buf, wpn, i > 0);
                }
            }
            if ((!arty && !marine && !intercept) || (arty && spd_v > 1)) {
                int value = wpn_v - arm_v;
                if (value <= -1) { // Defensive
                    if (sea_arty || (lrg_names && arm_v >= 8 && wpn_v*2 >= min(24, arm_v))) {
                        parse_chs_name(buf, Chassis[chs].defsv_name_lrg);
                    } else if (wpn_v >= 2) {
                        parse_chs_name(buf, Chassis[chs].defsv1_name);
                    } else {
                        parse_chs_name(buf, Chassis[chs].defsv2_name);
                    }
                } else if (value >= 1 || sea_arty) { // Offensive
                    if (sea_arty || (lrg_names && wpn_v >= 8 && arm_v*2 >= min(24, wpn_v))) {
                        parse_chs_name(buf, Chassis[chs].offsv_name_lrg);
                    } else if (triad != TRIAD_AIR && arm_v >= 2) {
                        parse_chs_name(buf, Chassis[chs].offsv1_name);
                    } else if (triad == TRIAD_AIR && !((wpn_v + arm_v) & 4)) {
                        parse_chs_name(buf, Chassis[chs].offsv1_name);
                    } else {
                        parse_chs_name(buf, Chassis[chs].offsv2_name);
                    }
                } else if (triad == TRIAD_LAND && spd_v == 1) { // Scout units
                    parse_chs_name(buf, Chassis[chs].offsv1_name);
                } else {
                    parse_chs_name(buf, Chassis[chs].offsv2_name);
                }
            }
            if (arty) {
                if ((wpn_v + arm_v) & 4) {
                    strncat(buf, label_or(543, "Artillery"), MaxProtoNameLen);
                } else {
                    strncat(buf, label_or(544, "Battery"), MaxProtoNameLen);
                }
                strncat(buf, " ", 2);
            }
            if (marine) {
                if ((wpn_v + arm_v) & 4) {
                    strncat(buf, label_or(614, "Marines"), MaxProtoNameLen);
                } else {
                    strncat(buf, label_or(615, "Invaders"), MaxProtoNameLen);
                }
                strncat(buf, " ", 2);
            }
            if (intercept) {
                if ((wpn_v + arm_v) & 4) {
                    parse_chs_name(buf, Chassis[chs].defsv1_name);
                } else {
                    parse_chs_name(buf, Chassis[chs].defsv2_name);
                }
            }
        } else {
            if (spd_v > 1) {
                parse_chs_name(buf, Chassis[chs].offsv1_name, Chassis[chs].offsv2_name);
            }
            if (abls & ABL_CARRIER) {
                parse_abl_name(buf, Ability[ABL_ID_CARRIER].abbreviation, i > 0);
            } else {
                parse_wpn_name(buf, wpn, i > 0);
            }
        }
        // Original: Thinker appends label_unit_reactor (thinker.ini); the
        // original game does not name reactors.
        strtrail(buf);
        if (strlen(buf) + 4 <= (size_t)MaxProtoNameLen) {
            break;
        }
    }
    strcpy_n(name, MaxProtoNameLen, buf);
}

void __cdecl name_proto(char* name, int UNUSED(unit_id), int UNUSED(faction_id), int chassis_id, int weapon_id,
int armor_id, int abilities, int reactor_id) {
    if (!name) {
        return;
    }
    if (!valid_components(chassis_id, weapon_id, armor_id, reactor_id)) {
        name[0] = '\0';
        return;
    }
    build_proto_name(name, (VehChassis)chassis_id, (VehWeapon)weapon_id, (VehArmor)armor_id,
        (VehAblFlag)abilities, (VehReactor)reactor_id);
}

int __cdecl mod_name_proto(char* name, int unit_id, int faction_id,
VehChassis chs, VehWeapon wpn, VehArmor arm, VehAblFlag abls, VehReactor rec) {
    if (!name) {
        return 0;
    }
    // Original: without new_unit_names, unit_id < 0 returns an empty name and
    // otherwise the exe name_proto is used; both paths name the design here.
    (void)unit_id;
    name_proto(name, unit_id, faction_id, chs, wpn, arm, abls, rec);
    return 0;
}

/* ---- Component choice and strength helpers (Thinker veh.cpp) ---- */

VehArmor best_armor(int faction_id, int max_cost) {
    int best_id = ARM_NO_ARMOR;
    for (int i = ARM_NO_ARMOR; i <= ARM_RESONANCE_8_ARMOR; i++) {
        if (has_tech(Armor[i].preq_tech, faction_id)) {
            int cost = Armor[i].cost;
            if (max_cost > 0 && (cost > max_cost || cost > Armor[i].defense_value + 3)) {
                continue;
            }
            int diff = Armor[i].defense_value - Armor[best_id].defense_value;
            if (diff > 0 || (diff == 0 && Armor[best_id].cost >= cost)) {
                best_id = i;
            }
        }
    }
    return (VehArmor)best_id;
}

VehWeapon best_weapon(int faction_id) {
    int best_id = WPN_HAND_WEAPONS;
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (has_tech(Weapon[i].preq_tech, faction_id)
        && Weapon[i].mode <= WMODE_MISSILE
        && Weapon[i].offense_value < 99
        && i != WPN_CONVENTIONAL_PAYLOAD) {
            int diff = Weapon[i].offense_value - Weapon[best_id].offense_value;
            if (diff > 0 || (diff == 0 && Weapon[best_id].cost >= Weapon[i].cost)) {
                best_id = i;
            }
        }
    }
    return (VehWeapon)best_id;
}

VehReactor best_reactor(int faction_id) {
    for (VehReactor r : {REC_SINGULARITY, REC_QUANTUM, REC_FUSION}) {
        if (has_tech(Reactor[r - 1].preq_tech, faction_id)) {
            return r;
        }
    }
    return REC_FISSION;
}

int proto_offense(int unit_id) {
    UNIT* u = &Units[unit_id];
    int atk_val = Weapon[u->weapon_id].offense_value;
    if (u->is_planet_buster()) {
        return atk_val * u->reactor_id;
    }
    return (conf.ignore_reactor_power || atk_val < 0 ?
        atk_val * REC_FISSION : atk_val * u->reactor_id);
}

int proto_defense(int unit_id) {
    UNIT* u = &Units[unit_id];
    int def_val = Armor[u->armor_id].defense_value;
    return (conf.ignore_reactor_power || def_val < 0 ?
        def_val * REC_FISSION : def_val * u->reactor_id);
}

/* ---- Thinker build.cpp: prototype choice for base production ---- */

static bool design_check_retool(BASE* base) {
    return base->plr_owner()
        && Factions[base->faction_id].diff_level > DIFF_SPECIALIST
        && Rules->retool_penalty_prod_change
        && Rules->retool_strictness != RETOOL_ALWAYS_FREE
        && !(base->state_flags & BSTATE_PRODUCTION_DONE)
        && base->minerals_accumulated > Rules->retool_exemption;
}

static int proto_extra_cost(int unit_id) {
    return unit_id >= MaxProtoFactionNum && !Units[unit_id].is_prototyped()
        ? prototype_factor(unit_id) : 0;
}

/*
Return true if unit2 is strictly better than unit1 in all circumstances (non PSI).
Disable random chance in prototype choices in these instances.
Weak: build.cpp belongs to WP6, which may provide the same definition.
*/
__attribute__((weak)) bool unit_is_better(int unit_id1, int unit_id2) {
    UNIT* u1 = &Units[unit_id1];
    UNIT* u2 = &Units[unit_id2];
    uint32_t abls_old = u1->ability_flags;
    uint32_t abls_new = u2->ability_flags;
    bool value = u1->cost >= u2->cost
        && proto_offense(unit_id1) >= 0
        && proto_offense(unit_id1) <= proto_offense(unit_id2)
        && proto_defense(unit_id1) <= proto_defense(unit_id2)
        && (conf.ignore_reactor_power || u1->reactor_id <= u2->reactor_id)
        && u1->speed() <= u2->speed()
        && !(((abls_old & abls_new) ^ abls_old) & ~ABL_SLOW)
        && (abls_old & ABL_ARTILLERY) == (abls_new & ABL_ARTILLERY)
        && (u2->triad() != TRIAD_AIR || (u1->chassis_id == u2->chassis_id
        && (abls_old & ABL_AIR_SUPERIORITY) == (abls_new & ABL_AIR_SUPERIORITY)));
    return value;
}

/* Weak: build.cpp belongs to WP6, which may provide the same definition. */
__attribute__((weak)) int unit_score(BASE* const base, int unit_id, int psi_score, int psi_atk, int psi_def, int flags) {
    Faction* f = &Factions[base->faction_id];
    AIPlans* p = &plans[base->faction_id];
    bool defend = flags & SC_Defend;
    const uint32_t specials[][2] = {
        {ABL_AAA, 4},
        {ABL_AIR_SUPERIORITY, 2},
        {ABL_ALGO_ENHANCEMENT, 5},
        {ABL_AMPHIBIOUS, (uint32_t)-2},
        {ABL_DROP_POD, 4},
        {ABL_EMPATH, 2},
        {ABL_TRANCE, 3},
        {ABL_SLOW, (uint32_t)-4},
        {ABL_TRAINED, 2},
        {ABL_COMM_JAMMER, 3},
        {ABL_ANTIGRAV_STRUTS, 3},
        {ABL_BLINK_DISPLACER, 3},
        {ABL_DEEP_PRESSURE_HULL, 2},
        {ABL_SUPER_TERRAFORMER, 8},
    };
    UNIT* u = &Units[unit_id];
    bool combat = u->plan <= PLAN_NAVAL_SUPERIORITY;
    int atk_val = proto_offense(unit_id);
    int def_val = proto_defense(unit_id);
    int v;
    if (u->is_missile() && !u->is_planet_buster() && atk_val > 0) {
        // Conv missiles might have nominally low attack rating
        atk_val = (atk_val + 7 * psi_atk) / 8;
    }
    if (defend) {
        v = (atk_val >= 0 ? 2 * atk_val : psi_atk + psi_score)
            + (def_val >= 0 ? 16 * (def_val + (def_val > 1)) : 12 * psi_def + 16 * psi_score);
    } else {
        v = (atk_val >= 0 ? 16 * (atk_val + (atk_val > 1)) : 12 * psi_atk + 16 * psi_score)
            + (def_val >= 0 ? 2 * def_val : psi_def + psi_score);
    }
    if (psi_score <= 0 && u->is_psi_unit()) {
        v += 8*(psi_score - 2);
    }
    if (combat && atk_val >= 0) {
        if (!atk_val) {
            v -= (defend ? 100 : 1000);
        }
        if ((defend && atk_val > def_val)
        || (!defend && atk_val < def_val)) {
            v -= 100;
        }
    }
    if (u->triad() != TRIAD_AIR) {
        v += (defend ? (combat ? 8 : 16) : 32) * u->speed();
    } else {
        v += clamp(4*u->speed(), 0, 48) + 16*(u->range() == 0);
        if (u->is_missile()) {
            v += (f->player_flags_ext & PFLAG_EXT_STRAT_LOTS_MISSILES ? 20 : 0);
            v -= 4 * p->missile_units;
        }
    }
    if (u->ability_flags & ABL_ARTILLERY) {
        if (conf.long_range_artillery > 0 && Rules->artillery_max_rng <= 4
        && !*MultiplayerActive && u->triad() == TRIAD_SEA && u->offense_value() > 0) {
            v += (conf.long_range_artillery > 1 ? 24 : 16);
        } else {
            v -= 8;
        }
        v += (f->player_flags_ext & PFLAG_EXT_STRAT_LOTS_ARTILLERY ? 20 : 0);
    }
    if (u->ability_flags & ABL_POLICE_2X && need_police(base->faction_id)) {
        v += (u->speed() > 1 ? 16 : 32);
        v += 8*min(4, (int)base->specialist_adjust);
    }
    if (u->ability_flags & ABL_CLEAN_REACTOR && u->plan <= unit_support_plan()
    && (f->SE_support_pending < 3 || base->mineral_consumption > 0)) {
        v += 16;
    }
    if (unit_id == base->production_id_last
    && Rules->retool_strictness >= RETOOL_FREE_PROJECT && design_check_retool(base)) {
        v += 200;
    }
    if (proto_extra_cost(unit_id) > 0) {
        v += clamp(base->mineral_surplus - 4, 0, 32);
        v += 40*base->has_fac_built(FAC_SKUNKWORKS);
        if (base->mineral_surplus >= p->median_limit) {
            v += 40*(atk_val > p->max_offense_value);
            v += 40*(def_val > p->max_defense_value);
        }
    }
    for (const auto& s : specials) {
        if (u->ability_flags & s[0]) {
            v += 8 * (int)s[1];
        }
    }
    // More detailed prototype cost calculations are skipped here
    int mins = max(4, base->mineral_surplus);
    int turns = (max(0, u->cost*10 - base->minerals_accumulated) + mins - 1) / mins;
    int score = v - turns*turns/10 - turns * (u->is_colony() ? 6 : 3)
        * (max(2, 8 - *CurrentTurn/16) + max(0, 2 - base->mineral_surplus/4));
    int cnt_num = 0;
    int cnt_cur = 0;
    int cnt_val = 0;
    if (flags & SC_EvalCount) {
        for (int i = *VehCount - 1; i >= 0; --i) {
            if (Vehs[i].faction_id == base->faction_id) {
                cnt_num++;
                cnt_cur += (Vehs[i].unit_id == unit_id);
            }
        }
        for (int i = *BaseCount - 1; i >= 0; --i) {
            BASE* b = &Bases[i];
            if (b->faction_id == base->faction_id && b != base && b->item() >= 0) {
                cnt_num++;
                cnt_cur += (b->item() == unit_id);
            }
        }
        cnt_val = clamp(4 * cnt_num, 0, 256)
            * max(0, min(cnt_cur - 4 - cnt_num / (defend ? 4 : 8), cnt_num/2)) / max(1, cnt_num);
        score -= cnt_val;
    }
    return score;
}

/*
Find the best prototype for base production when weighted against cost given the triad
and type constraints. For any combat-capable unit, mode is set to WMODE_COMBAT.
*/
int find_proto(int base_id, TriadFlag triad, VehWeaponMode mode, bool defend) {
    if (base_id < 0 || base_id >= *BaseCount) { // Fix: bounds check instead of assert
        return -FAC_STOCKPILE_ENERGY;
    }
    BASE* base = &Bases[base_id];
    int faction_id = base->faction_id;
    int gov = base->gov_config();
    debug("find_proto faction: %d triad: %d mode: %d defend: %d\n", faction_id, triad, mode, defend);

    int psi_score = (gov & GOV_MAY_PROD_NATIVE ? plans[faction_id].psi_score
        + has_fac_built(FAC_BROOD_PIT, base_id)
        + has_fac_built(FAC_BIOLOGY_LAB, base_id)
        + has_fac_built(FAC_CENTAURI_PRESERVE, base_id)
        + has_fac_built(FAC_TEMPLE_OF_PLANET, base_id) : 0);
    bool prototypes = (gov & GOV_MAY_PROD_PROTOTYPE) || has_fac_built(FAC_SKUNKWORKS, base_id);
    bool combat = (mode == WMODE_COMBAT);
    bool pacifism = combat
        && base_can_riot(base_id, true)
        && base->SE_police(SE_Pending) <= -3
        && base->drone_total + base->specialist_adjust >= base->talent_total;
    int best_id = -FAC_STOCKPILE_ENERGY;
    int best_val = -10000;
    int psi_atk = 1;
    int psi_def = 1;
    std::vector<int> choices;

    for (int id = 0; id < MaxProtoNum; id++) {
        UNIT* u = &Units[id];
        if ((id < MaxProtoFactionNum || id / MaxProtoFactionNum == faction_id)
        && veh_avail(id, faction_id, base_id) && !u->is_planet_buster()) {
            psi_atk = max(psi_atk, proto_offense(id));
            psi_def = max(psi_def, proto_defense(id));
            if ((1 << u->triad()) & triad) {
                if (!prototypes && proto_extra_cost(id) > 0) {
                    continue;
                }
                if ((!combat && Weapon[u->weapon_id].mode != mode)
                || (combat && (u->offense_value() == 0 && (!defend || u->plan > PLAN_RECON)))
                || (u->is_psi_unit() && !(gov & GOV_MAY_PROD_NATIVE))) {
                    continue;
                }
                if (combat && u->triad() == TRIAD_AIR) {
                    bool intercept = has_abil(id, ABL_AIR_SUPERIORITY);
                    if ((!intercept && pacifism)
                    || (!intercept && !(gov & GOV_MAY_PROD_AIR_COMBAT))
                    || (intercept && !(gov & GOV_MAY_PROD_AIR_DEFENSE))) {
                        continue;
                    }
                }
                choices.push_back(id);
            }
        }
    }
    int flags = (defend ? SC_Defend : 0) | (combat ? SC_EvalCount : 0);
    for (int id : choices) {
        int val = unit_score(base, id, psi_score, psi_atk, psi_def, flags);
        if (best_id < 0 || unit_is_better(best_id, id) || random(128) > 64 + best_val - val) {
            best_id = id;
            best_val = val;
        }
    }
    return best_id;
}

/* ---------------------------------------------------------------------------
 * Prototype bookkeeping (terranx.exe, written fresh)
 * ------------------------------------------------------------------------- */

void __cdecl retire_proto(int unit_id, int faction_id) {
    if (unit_id < 0 || unit_id >= MaxProtoNum || faction_id < 0 || faction_id >= MaxPlayerNum) {
        return;
    }
    UNIT* u = &Units[unit_id];
    u->obsolete_factions |= (1 << faction_id);
    if (unit_id >= MaxProtoFactionNum && unit_id / MaxProtoFactionNum == faction_id
    && !veh_count(faction_id, unit_id) && !proto_in_queue(unit_id, faction_id)) {
        u->unit_flags &= ~UNIT_ACTIVE;
    }
}

/* Same role: both combat (mode <= missile) or the same non-combat mode, same triad. */
static bool same_role(int unit_id1, int unit_id2) {
    UNIT* u1 = &Units[unit_id1];
    UNIT* u2 = &Units[unit_id2];
    if (u1->triad() != u2->triad() || u1->is_missile() != u2->is_missile()) {
        return false;
    }
    int m1 = u1->weapon_mode();
    int m2 = u2->weapon_mode();
    if (m1 <= WMODE_MISSILE && m2 <= WMODE_MISSILE) {
        return true;
    }
    return m1 == m2;
}

void __cdecl prune_protos(int faction_id, int unit_id, int flag) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum || unit_id < 0 || unit_id >= MaxProtoNum) {
        return;
    }
    if (is_human(faction_id) && !flag && !(*GameMorePreferences & MPREF_BSC_AUTO_PRUNE_OBS_VEH)) {
        return;
    }
    for (int id = 0; id < MaxProtoNum; id++) {
        if (id == unit_id || !proto_known(id, faction_id)
        || (Units[id].obsolete_factions & (1 << faction_id))) {
            continue;
        }
        if (same_role(id, unit_id) && unit_is_better(id, unit_id)
        && !is_native_unit(id)) {
            debug("prune_proto %d %d %s -> %s\n", *CurrentTurn, faction_id, Units[id].name, Units[unit_id].name);
            retire_proto(id, faction_id);
        }
    }
}

void __cdecl upgrade_prototypes(int faction_id, int unit_id) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum
    || unit_id < MaxProtoFactionNum || unit_id / MaxProtoFactionNum != faction_id) {
        return;
    }
    Units[unit_id].unit_flags &= ~UNIT_UNK_100;
    for (int id = 0; id < MaxProtoNum; id++) {
        if (id == unit_id || !(Units[id].unit_flags & UNIT_ACTIVE)) {
            continue;
        }
        if ((id < MaxProtoFactionNum || id / MaxProtoFactionNum == faction_id)
        && veh_count(faction_id, id) > 0 && same_role(id, unit_id)
        && unit_is_better(id, unit_id) && !is_native_unit(id)) {
            mod_upgrade_prototype(faction_id, unit_id, id, 1);
        }
    }
}

void __cdecl upgrade_any_prototypes(int faction_id) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum) {
        return;
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int id = faction_id * MaxProtoFactionNum + i;
        UNIT* u = &Units[id];
        if ((u->unit_flags & UNIT_ACTIVE) && (u->unit_flags & UNIT_UNK_100)
        && !(u->obsolete_factions & (1 << faction_id))) {
            upgrade_prototypes(faction_id, id);
        }
    }
}

/* An available design at least as good in every component (same chassis). */
static bool design_covered(int faction_id, int chs, int wpn, int arm, int rec) {
    for (int id = 0; id < MaxProtoNum; id++) {
        if (!proto_known(id, faction_id) || (Units[id].obsolete_factions & (1 << faction_id))) {
            continue;
        }
        UNIT* u = &Units[id];
        if (u->chassis_id != chs || u->reactor_id < rec) {
            continue;
        }
        bool wpn_ok = (u->weapon_id == wpn)
            || (Weapon[wpn].mode <= WMODE_MISSILE && Weapon[u->weapon_id].mode == Weapon[wpn].mode
            && Weapon[u->weapon_id].offense_value >= Weapon[wpn].offense_value
            && Weapon[wpn].offense_value > 0);
        bool arm_ok = (u->armor_id == arm)
            || (Armor[arm].defense_value > 0 && Armor[u->armor_id].defense_value >= Armor[arm].defense_value);
        if (wpn_ok && arm_ok && (u->ability_flags & ~ABL_SLOW) == 0) {
            return true;
        }
    }
    return false;
}

static void consider(int faction_id, int chs, int wpn, int arm, int rec) {
    if (!valid_components(chs, wpn, arm, rec)
    || !has_tech(Chassis[chs].preq_tech, faction_id)
    || !has_tech(Weapon[wpn].preq_tech, faction_id)
    || !has_tech(Armor[arm].preq_tech, faction_id)
    || design_covered(faction_id, chs, wpn, arm, rec)) {
        return;
    }
    mod_propose_proto(faction_id, (VehChassis)chs, (VehWeapon)wpn, (VehArmor)arm,
        ABL_NONE, (VehReactor)rec, PLAN_AUTO_CALCULATE, NULL);
}

/*
Fresh replacement for the original consider_designs: propose the standard
designs of the faction's best components (best weapon, best armor, best
reactor) when no available design already covers them:
land infantry attacker/defender/combined, fastest land attacker and
non-combat units, sea combat and transport, air attacker.
*/
void __cdecl consider_designs(int faction_id) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum) {
        return;
    }
    int wpn = best_weapon(faction_id);
    int arm = best_armor(faction_id, 0);
    int rec = best_reactor(faction_id);
    int fast_land = -1;
    int sea = -1;
    int air = -1;
    for (int i = 0; i < MaxChassisNum; i++) {
        CChassis& c = Chassis[i];
        if (!has_tech(c.preq_tech, faction_id) || c.missile) {
            continue;
        }
        if (c.triad == TRIAD_LAND && c.speed > 1 && (fast_land < 0 || c.speed > Chassis[fast_land].speed)) {
            fast_land = i;
        } else if (c.triad == TRIAD_SEA && (sea < 0 || c.speed > Chassis[sea].speed)) {
            sea = i;
        } else if (c.triad == TRIAD_AIR && c.range > 0 && (air < 0 || c.speed > Chassis[air].speed)) {
            air = i;
        }
    }
    bool good_wpn = Weapon[wpn].offense_value > 1;
    bool good_arm = Armor[arm].defense_value > 1;
    if (good_arm) {
        consider(faction_id, CHS_INFANTRY, WPN_HAND_WEAPONS, arm, rec);
    }
    if (good_wpn) {
        consider(faction_id, CHS_INFANTRY, wpn, ARM_NO_ARMOR, rec);
    }
    if (good_wpn && good_arm) {
        consider(faction_id, CHS_INFANTRY, wpn, arm, rec);
    }
    if (fast_land >= 0) {
        if (good_wpn) {
            consider(faction_id, fast_land, wpn, ARM_NO_ARMOR, rec);
        }
        consider(faction_id, fast_land, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, rec);
        consider(faction_id, fast_land, WPN_PROBE_TEAM, ARM_NO_ARMOR, rec);
    }
    if (sea >= 0) {
        if (good_wpn) {
            consider(faction_id, sea, wpn, arm, rec);
        }
        consider(faction_id, sea, WPN_TROOP_TRANSPORT, ARM_NO_ARMOR, rec);
    }
    if (air >= 0 && good_wpn) {
        consider(faction_id, air, wpn, ARM_NO_ARMOR, rec);
    }
}

void __cdecl design_new_veh(int faction_id, int unit_id) {
    // Original: opens DesignWin on unit_id for the local player.
    engine_notify(EV_MESSAGE, faction_id, unit_id, 0, 0, -1, -1, "design_new_veh");
}

static std::vector<int> s_scanned_protos;

void __cdecl scan_prototypes(int faction_id, int faction_id_2) {
    s_scanned_protos.clear();
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || faction_id_2 <= 0
    || faction_id_2 >= MaxPlayerNum || faction_id == faction_id_2) {
        return;
    }
    for (int i = 0; i < MaxProtoFactionNum; i++) {
        int id = faction_id_2 * MaxProtoFactionNum + i;
        UNIT* u = &Units[id];
        if (!(u->unit_flags & UNIT_ACTIVE) || !(u->unit_flags & UNIT_PROTOTYPED)
        || (u->obsolete_factions & (1 << faction_id_2))
        || !valid_components(u->chassis_id, u->weapon_id, u->armor_id, u->reactor_id)) {
            continue;
        }
        bool can_build = has_tech(Chassis[u->chassis_id].preq_tech, faction_id)
            && has_tech(Weapon[u->weapon_id].preq_tech, faction_id)
            && has_tech(Armor[u->armor_id].preq_tech, faction_id)
            && has_tech(Reactor[u->reactor_id - 1].preq_tech, faction_id);
        for (int j = 0; j < MaxAbilityNum && can_build; j++) {
            if (u->ability_flags & (1u << j)) {
                can_build = has_tech(Ability[j].preq_tech, faction_id);
            }
        }
        if (!can_build) {
            continue;
        }
        bool has_equal = false;
        for (int k = 0; k < MaxProtoNum && !has_equal; k++) {
            if (!proto_known(k, faction_id)) {
                continue;
            }
            UNIT* v = &Units[k];
            has_equal = v->chassis_id == u->chassis_id && v->weapon_id == u->weapon_id
                && v->armor_id == u->armor_id && v->ability_flags == u->ability_flags
                && v->reactor_id == u->reactor_id;
        }
        if (!has_equal) {
            s_scanned_protos.push_back(id);
        }
    }
}

int scanned_prototypes(int* unit_ids, int max_ids) {
    int n = (int)s_scanned_protos.size();
    for (int i = 0; i < n && i < max_ids && unit_ids; i++) {
        unit_ids[i] = s_scanned_protos[i];
    }
    return n;
}

} // namespace engine
} // namespace alpha
