/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: faction strategy and capabilities.
 * Ported from Thinker Mod src/faction_event.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations from Thinker are marked "Fix:" (behaviour) or "Original:"
 * (replaced executable or Windows code) and listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {
namespace ai {

} // namespace ai

void __cdecl enemy_capabilities(int faction_id) {
    using namespace ai;
    Faction& plr = Factions[faction_id];
    plr.best_psi_land_offense = veh_avail(BSC_MIND_WORMS, faction_id, -1)
        ? weap_strat(WPN_PSI_ATTACK, faction_id) : 0;
    plr.best_weapon_value = 1;
    for (int weapon_id = 0; weapon_id < MaxWeaponNum; weapon_id++) {
        if (has_tech(Weapon[weapon_id].preq_tech, faction_id)
        && Weapon[weapon_id].offense_value < 99) {
            int value = weap_strat(weapon_id, faction_id);
            if (Weapon[weapon_id].offense_value < 0 && value >= plr.best_psi_land_offense) {
                plr.best_psi_land_offense = value;
            }
            if (value >= plr.best_weapon_value) {
                plr.best_weapon_value = value;
            }
        }
    }
    plr.best_psi_land_defense = veh_avail(BSC_MIND_WORMS, faction_id, -1)
        ? arm_strat(ARM_PSI_DEFENSE, faction_id) : 0;
    plr.best_armor_value = 1;
    for (int armor_id = 0; armor_id < MaxArmorNum; armor_id++) {
        if (has_tech(Armor[armor_id].preq_tech, faction_id)) {
            int value = arm_strat(armor_id, faction_id);
            if (Armor[armor_id].defense_value < 0 && value >= plr.best_psi_land_defense) {
                plr.best_psi_land_defense = value;
            }
            if (value >= plr.best_armor_value) {
                plr.best_armor_value = value;
            }
        }
    }
    plr.best_land_speed = 1;
    for (int chassis_id = 0; chassis_id < MaxChassisNum; chassis_id++) {
        if (has_tech(Chassis[chassis_id].preq_tech, faction_id)
        && Chassis[chassis_id].triad == TRIAD_LAND) {
            int speed = Chassis[chassis_id].speed;
            if (speed >= plr.best_land_speed) {
                plr.best_land_speed = speed;
            }
        }
    }
    plr.enemy_best_land_speed = 0;
    plr.enemy_best_armor_value = 0;
    plr.enemy_best_weapon_value = 0;
    plr.enemy_best_psi_land_defense = 0;
    plr.enemy_best_psi_land_offense = 0;
    for (int iter = 0; iter < 4; iter++) {
        if (plr.enemy_best_weapon_value) {
            return;
        }
        for (int enemy_id = 1; enemy_id < MaxPlayerNum; enemy_id++) {
            uint32_t status = plr.diplo_status[enemy_id];
            if (enemy_id == faction_id || (status & DIPLO_PACT)) {
                continue;
            }
            if (iter == 0 && !(status & DIPLO_VENDETTA)) {
                continue;
            }
            if (iter <= 1 && (status & DIPLO_TREATY)) {
                continue;
            }
            if (iter > 2 || (status & DIPLO_COMMLINK)) {
                Faction& enemy = Factions[enemy_id];
                if (plr.enemy_best_weapon_value <= enemy.best_weapon_value) {
                    plr.enemy_best_weapon_value = enemy.best_weapon_value;
                }
                if (plr.enemy_best_armor_value <= enemy.best_armor_value) {
                    plr.enemy_best_armor_value = enemy.best_armor_value;
                }
                if (plr.enemy_best_land_speed <= enemy.best_land_speed) {
                    plr.enemy_best_land_speed = enemy.best_land_speed;
                }
                if (plr.enemy_best_psi_land_offense <= enemy.best_psi_land_offense) {
                    plr.enemy_best_psi_land_offense = enemy.best_psi_land_offense;
                }
                if (plr.enemy_best_psi_land_defense <= enemy.best_psi_land_defense) {
                    plr.enemy_best_psi_land_defense = enemy.best_psi_land_defense;
                }
            }
        }
    }
}

namespace ai {

} // namespace ai

void __cdecl enemy_strategy(int faction_id) {
    using namespace ai;
    Faction& plr = Factions[faction_id];
    int faction_bit = 1 << faction_id;
    int region_formers[MaxRegionNum];
    int goal_assigned[75];
    int waiting_units = 0;
    int transport_count = 0;
    int max_cargo = 2;

    plr.total_combat_units = 0;
    plr.base_count = 0;
    plr.mil_strength_1 = 0;
    plr.mil_strength_2 = 0;
    plr.pop_total = 0;
    plr.unk_70 = 0;
    plr.planet_busters = 0;
    plr.unk_71 = 0;
    plr.unk_72 = 0;

    for (int i = 0; i < MaxRegionNum; i++) {
        plr.region_total_combat_units[i] = 0;
        plr.region_total_bases[i] = 0;
        plr.region_force_rating[i] = 0;
        plr.region_flags[i] &= ((faction_id + *CurrentTurn + i) & 0xF) != 0 ? REGION_UNK_8 : 0;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        plr.units_active[i] = 0;
        plr.units_queue[i] = 0;
    }
    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        BASE& base = Bases[base_id];
        int x = base.x;
        int y = base.y;
        if (base.faction_id == faction_id) {
            base.state_flags &= ~(BSTATE_UNK_4000000|BSTATE_UNK_8000000);
            if (base.ai_plan_status) {
                base.ai_plan_status--;
            }
            plr.base_count++;
            plr.pop_total += base.pop_size;
            int stack_id = stack_fix(veh_at(x, y));
            if (!stack_check(stack_id, 14, faction_id, -1, -1)
            && !stack_check(stack_id, 2, PLAN_RECON, faction_id, -1)) {
                add_goal(faction_id, AI_GOAL_DEFEND, 4, x, y, -1);
            }
            for (int i = 0; i <= base.queue_size; i++) {
                if (base.queue_items[i] >= 0) {
                    plr.units_queue[base.queue_items[i]]++;
                }
            }
            int region = mapsq(x, y)->region;
            plr.region_total_bases[region]++;
            if (is_coast(x, y, 0)) {
                plr.region_flags[region] |= REGION_UNK_80;
            }
            if (((base_id + *CurrentTurn) & 3) == 0) {
                base.event_flags &= ~0xFF; // TODO BaseEvent
            }
            continue;
        }
        if (!(base.visibility & faction_bit)) {
            continue;
        }
        uint32_t status = plr.diplo_status[base.faction_id];
        if (!(status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800)) || (status & DIPLO_PACT)) {
            continue;
        }
        int priority = 1;
        MAP* sq = mapsq(x, y);
        if (sq->veh_who() < 0) {
            priority = 2;
            add_goal(faction_id, AI_GOAL_DEFEND, 2, x, y, -1);
            add_goal(faction_id, AI_GOAL_SCOUT, 2, x, y, -1);
        }
        if (base.faction_id_former == faction_id) {
            priority++;
        }
        if (plr.region_total_bases[sq->region]) {
            priority++;
        }
        if (!has_fac_built(FAC_PERIMETER_DEFENSE, base_id)) {
            priority++;
        }
        if (base.queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE) {
            priority *= 4;
        }
        if (has_fac_built(FAC_HEADQUARTERS, base_id)) {
            priority++;
            if (plr.player_flags & PFLAG_STRAT_ATK_ENEMY_HQ) {
                priority *= 2;
            }
            if (Factions[base.faction_id].corner_market_active()) {
                priority *= 8;
            }
        }
        if ((plr.base_id_attack_target == base_id && (plr.diplo_status[base.faction_id] & DIPLO_VENDETTA))
        || is_objective(base_id)) {
            priority *= 4;
        }
        add_goal(faction_id, AI_GOAL_ATTACK, priority, x, y, -1);
        if (on_map(base.unk_x, base.unk_y)) {
            int target_base = base_find(base.unk_x, base.unk_y);
            if (target_base >= 0 && Bases[target_base].faction_id == base.faction_id) {
                if (terraform_cost(base.unk_x, base.unk_y, faction_id) <= plr.energy_credits / 8) {
                    add_goal(faction_id, AI_GOAL_UNK_4, 2, base.unk_x, base.unk_y, -1);
                }
            }
        }
    }

    if (!is_human(faction_id)) {
        for (int base_id = 0; base_id < *BaseCount; base_id++) {
            if (Bases[base_id].faction_id == faction_id
            && Bases[base_id].queue_items[0] == -FAC_STOCKPILE_ENERGY) {
                mod_base_reset(base_id, 0);
            }
        }
    }

    memset(region_formers, 0, sizeof(region_formers));
    enemy_capabilities(faction_id);
    plr.unk_68 = 0;
    plr.unk_66 = 0;
    plr.unk_65 = 0;
    plr.unk_64 = 0;
    plr.unk_106 = 0;
    plr.unk_105 = 0;

    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        if (veh_id >= *VehCount) {
            continue;
        }
        VEH& veh = Vehs[veh_id];
        MAP* veh_sq = mapsq(veh.x, veh.y);
        assert(veh_sq);
        int veh_base_who = (veh_sq ? veh_sq->base_who() : -1);
        int unit_id = veh.unit_id;
        UNIT& unit = Units[unit_id];
        if (veh.faction_id != faction_id) {
            uint32_t status = plr.diplo_status[veh.faction_id];
            if ((status & DIPLO_COMMLINK)
            && (!(status & (DIPLO_TREATY|DIPLO_PACT))
            || (status & (DIPLO_UNK_800|DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE)))) {
                if (unit.plan == PLAN_NAVAL_TRANSPORT) {
                    plr.unk_64++;
                }
                if (unit.plan == PLAN_NAVAL_SUPERIORITY) {
                    plr.unk_65 += unit.reactor_id;
                }
                if (unit.triad() == TRIAD_AIR) {
                    if (unit.plan == PLAN_OFFENSE && !unit.is_missile()
                    && weap_val(unit_id, faction_id) >= plr.enemy_best_armor_value) {
                        plr.unk_66++;
                    }
                    if (unit.is_missile() && !planet_buster(veh_id)) {
                        plr.unk_68++;
                    }
                }
                if (unit.plan == PLAN_TERRAFORM && unit.triad() == TRIAD_SEA) {
                    if (veh_sq && is_ocean(veh_sq) && veh_base_who < 0
                    && ((veh.faction_id == faction_id && !veh.is_invisible_lurker())
                    || (faction_bit & veh.visibility))) {
                        add_goal(faction_id, AI_GOAL_UNK_1,
                            (plr.player_flags & PFLAG_UNK_80000) ? 4 : 2, veh.x, veh.y, -1);
                    }
                }
            }
            continue;
        }
        if (!veh_sq) {
            continue;
        }
        int region = veh_sq->region;
        bool on_water = is_ocean(veh_sq);
        int plan = (uint8_t)unit.plan;
        int triad = unit.triad();
        plr.units_active[unit_id]++;
        if (veh.year_end_lurking) {
            veh.year_end_lurking--;
            if (!veh.year_end_lurking) {
                veh.flags &= ~(VFLAG_INVISIBLE|VFLAG_LURKER);
            }
        }
        if (!is_human(faction_id)
        && (faction_bit & unit.obsolete_factions)
        && conf.factions_enabled < faction_id) {
            // Original: Console_upgrade(MapWin, veh_id) only runs for factions not managed
            // by Thinker (conf.factions_enabled < faction_id); the UI upgrade dialog is not ported.
            // Fix: remove additional code that considered disbanding units if the upgrade failed
            // because incorrect parameters to arm_val compared undefined memory locations.
        }
        if (!is_human(faction_id)
        && ((*CurrentTurn + veh_id) & 3) != 0
        && *CurrentTurn > 25
        && unit_id == BSC_SCOUT_PATROL
        && (plr.region_base_plan[region] == PLAN_NAVAL_TRANSPORT || veh.order == ORDER_HOLD)) {
            if (!on_water
            && (veh_base_who < 0 || stack_check(veh_id, 14, faction_id, -1, -1) > 1)
            && plr.region_base_plan[region] != PLAN_DEFENSE
            && !zoc_sea(veh.x, veh.y, faction_id)
            && veh.status_icon != '1') {
                kill(veh_id);
                continue;
            }
        }
        veh.state &= ~(VSTATE_UNK_10000000|VSTATE_UNK_20000000|VSTATE_MADE_AIRDROP|VSTATE_HAS_MOVED);
        if (!is_human(faction_id) && veh.order == ORDER_SENTRY_BOARD
        && !(triad == TRIAD_LAND && on_water)) {
            veh.order = ORDER_NONE;
        }
        if (on_water) {
            if (triad == TRIAD_LAND && veh_base_who < 0) {
                if (!stack_check(veh_id, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                    if (!(veh.flags & VFLAG_IS_OBJECTIVE)) {
                        veh_kill(veh_id);
                    }
                    continue;
                }
                if (veh.order != ORDER_SENTRY_BOARD) {
                    sleep(veh_id);
                }
            }
        } else if (triad == TRIAD_SEA && veh_base_who < 0) {
            if (!(veh.flags & VFLAG_IS_OBJECTIVE)) {
                veh_kill(veh_id);
            }
            continue;
        }
        if (plan <= PLAN_COMBAT && !(veh.state & VSTATE_UNK_40000)) {
            int garrison_base_id = base_at(veh.x, veh.y);
            if (garrison_base_id < 0 && veh.status_icon == 'F') {
                garrison_base_id = veh.home_base_id;
            }
            if (garrison_base_id >= 0) {
                int garrison = garrison_check(garrison_base_id);
                int needed = garrison_check(garrison_base_id);
                if (needed > 5) {
                    needed--;
                }
                if (needed > 2) {
                    needed--;
                }
                int base_stack = veh_at(Bases[garrison_base_id].x, Bases[garrison_base_id].y);
                int designated = stack_check(base_stack, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, faction_id);
                if (garrison > needed + designated) {
                    veh.state |= VSTATE_UNK_10000000;
                }
            }
        }
        if (veh.status_icon == 'F' && plan == PLAN_DEFENSE) {
            int home_id = veh.home_base_id;
            if (home_id >= 0) {
                if (Bases[home_id].x != veh.x || Bases[home_id].y != veh.y) {
                    Bases[home_id].state_flags |= BSTATE_UNK_4000000;
                }
            }
        } else {
            switch (plan) {
                case PLAN_TERRAFORM: {
                    veh.flags &= ~VFLAG_UNK_800;
                    region_formers[region]++;
                    int home_id = veh.home_base_id;
                    if (home_id >= 0) {
                        Bases[home_id].state_flags |= (triad != TRIAD_SEA ? BSTATE_UNK_20 : BSTATE_UNK_80000);
                    }
                    // Fix: remove additional code that was inconsistently used to reassign AI formers
                    // to nearby bases without checking the base ownership. That code did not first
                    // move units to the base tile and only picked bases that had mineral_surplus < 2.
                    break;
                }
                case PLAN_COLONY:
                    if (plr.region_flags[region] & REGION_UNK_2) {
                        plr.region_flags[region] |= REGION_UNK_800;
                    } else {
                        plr.region_flags[region] |= REGION_UNK_2;
                    }
                    break;
                case PLAN_PROBE:
                    plr.region_flags[region] |= REGION_UNK_1000;
                    break;
                case PLAN_RECON:
                    if (veh_base_who < 0) {
                        plr.region_flags[region] |= REGION_UNK_2000;
                    }
                    break;
            }
        }
        if (plan <= PLAN_NAVAL_TRANSPORT) {
            plr.total_combat_units++;
            if (region >= 0 && triad == TRIAD_LAND && !on_water) {
                plr.region_total_combat_units[region]++;
            }
        }
        if (triad == TRIAD_SEA) {
            plr.unk_70++;
        } else if (triad == TRIAD_AIR) {
            if (plan == PLAN_OFFENSE) {
                plr.unk_105++;
            } else if (plan == PLAN_AIR_SUPERIORITY) {
                plr.unk_106++;
            }
        }
        if (plan == PLAN_NAVAL_TRANSPORT) {
            transport_count++;
            if (max_cargo <= veh_cargo(veh_id)) {
                max_cargo = veh_cargo(veh_id);
            }
        }

        if (planet_buster(veh_id)) {
            plr.planet_busters++;
            continue;
        }
        int wpn = weap_strat(veh.weapon_type(), faction_id);
        int arm = arm_strat(veh.armor_type(), faction_id);
        int morale = morale_veh(veh_id, 0, 0) /* WP4 (Thinker mod_morale_veh) */ + 6;
        plr.mil_strength_1 += morale * (wpn + arm) / 8;
        if (wpn > arm || veh.order != ORDER_HOLD) {
            int atk_val = wpn * morale / 8;
            plr.mil_strength_2 += atk_val;
            if (region >= 0) {
                plr.region_force_rating[region] += atk_val;
            }
        }
        uint32_t target_state = veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000);
        if (target_state == (VSTATE_UNK_40000|VSTATE_UNK_20000)) {
            waiting_units++;
            continue;
        }
        if (target_state != VSTATE_UNK_40000) {
            continue;
        }
        MAP* nsq = mapsq(plr.target_x, plr.target_y);
        if (!nsq || region != nsq->region) {
            veh.state |= VSTATE_UNK_20000;
            waiting_units++;
            continue;
        }
        plr.unk_71++;
        if (plan == PLAN_PROBE) {
            plr.unk_72 |= 2;
        } else if (arm > wpn) {
            plr.unk_72 |= 1;
        }
    }

    plr.unk_70 += (transport_count + (max_cargo << 11)) << 11;
    int max_rival_cargo = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id) {
            uint32_t status = plr.diplo_status[i];
            if ((status & DIPLO_COMMLINK) && !(status & DIPLO_PACT)
            && max_rival_cargo <= Factions[i].unk_70 / 0x400000) {
                max_rival_cargo = Factions[i].unk_70 / 0x400000;
            }
        }
    }
    plr.unk_64 += max_rival_cargo << 11;

    if (transport_count > 1) {
        for (int i = 0; i < *VehCount; i++) {
            int unit_id = Vehs[i].unit_id;
            if (Vehs[i].faction_id == faction_id
            && Units[unit_id].triad() == TRIAD_SEA
            && Units[unit_id].plan == PLAN_NAVAL_TRANSPORT
            && (unit_id == BSC_UNITY_FOIL || plr.units_active[unit_id] >= 2)) {
                Vehs[i].state |= VSTATE_UNK_20000000;
                break;
            }
        }
    }

    for (int i = 0; i < *VehCount; i++) {
        VEH& veh = Vehs[i];
        if (veh.faction_id == faction_id && veh.plan() == PLAN_TERRAFORM) {
            unsigned region = mapsq(veh.x, veh.y)->region;
            if (region < MaxRegionLandNum) {
                if (region_formers[region] >= 3 && plr.region_total_bases[region] >= 3) {
                    veh.flags |= VFLAG_UNK_800;
                    if (veh.home_base_id >= 0) {
                        Bases[veh.home_base_id].state_flags &= ~BSTATE_UNK_20;
                    }
                    region_formers[region] = 0;
                }
            }
        }
    }

    int target_region = -1;
    bool skip_targeting = is_human(faction_id);
    if (plr.target_x >= 0) {
        int target_base = base_at(plr.target_x, plr.target_y);
        MAP* sq = mapsq(plr.target_x, plr.target_y);
        target_region = (sq ? sq->region : -1);
        assert(sq);
        if (target_base < 0 || Bases[target_base].faction_id != faction_id) {
            skip_targeting = true;
        } else {
            int target_stack = stack_fix(veh_at(Bases[target_base].x, Bases[target_base].y));
            if (target_stack >= 0) {
                int defenders = stack_check(target_stack, 11, VSTATE_UNK_40000|VSTATE_UNK_20000, VSTATE_UNK_40000, -1);
                int needed = clamp(2 * plr.region_total_bases[target_region] / 3, 3, 8);
                if (defenders - bit_count(plr.unk_72) >= needed) {
                    skip_targeting = true;
                }
                unsigned region_plan = plr.region_base_plan[target_region];
                if (region_plan == PLAN_DEFENSE || (plr.region_flags[target_region] & (REGION_UNK_4000|REGION_UNK_200))) {
                    skip_targeting = true;
                }
                if (region_plan > PLAN_DEFENSE
                && stack_check(target_stack, 2, PLAN_NAVAL_TRANSPORT, -1, -1)) {
                    if (defenders >= 2 * (plr.unk_70 / 0x400000)) {
                        skip_targeting = true;
                    }
                    if (stack_check(target_stack, 2, PLAN_NAVAL_TRANSPORT, -1, -1) > 1) {
                        int ground_count = stack_check(target_stack, 8, -1, -1, -1);
                        int land_count = stack_check(target_stack, 3, TRIAD_LAND, -1, -1);
                        if (defenders >= land_count + ground_count) {
                            skip_targeting = true;
                        }
                    }
                }
            }
            if (!skip_targeting) {
                unsigned region_plan = plr.region_base_plan[target_region];
                if (region_plan <= PLAN_DEFENSE) {
                    plr.player_flags &= ~PFLAG_UNK_800;
                } else if (region_plan == PLAN_COLONY
                || !is_coast(Bases[target_base].x, Bases[target_base].y, 0)) {
                    skip_targeting = true;
                } else {
                    plr.player_flags |= PFLAG_UNK_800;
                }
            }
        }
    }
    if (skip_targeting) {
        for (int i = 0; i < *VehCount; i++) {
            if (Vehs[i].faction_id == faction_id && (Vehs[i].state & VSTATE_UNK_40000)) {
                Vehs[i].state |= VSTATE_UNK_20000;
            }
        }
        plr.target_x = -1;
        plr.target_y = -1;
        plr.unk_71 = 0;
        plr.unk_72 = 0;
    }
    int best_value = 0;
    int best_region = -1;
    for (int region = 1; region < MaxRegionNum; region++) {
        plr.region_total_offensive_units[region] = 0;
        if (bad_reg(region) && !plr.region_total_bases[region]) {
            plr.region_base_plan[region] = PLAN_COLONY;
            continue;
        }
        int old_plan = plr.region_base_plan[region];
        int plan_flags = 0;
        int threat_count = 0;
        int weak_count = 0;
        int factions_present = 0;
        int contact_count = 0;
        int land_contact_count = 0;
        bool conflict = false;
        int my_units = plr.region_total_combat_units[region];
        int my_force = plr.region_force_rating[region];
        int my_bases = plr.region_total_bases[region];

        for (int enemy_id = 1; enemy_id < 8; enemy_id++) {
            Faction& enemy = Factions[enemy_id];
            uint32_t enemy_status = plr.diplo_status[enemy_id];
            int enemy_units = enemy.region_total_combat_units[region];
            int enemy_force = enemy.region_force_rating[region];
            int enemy_bases = enemy.region_total_bases[region];
            if (enemy_status & DIPLO_PACT) {
                enemy_units *= 2;
                enemy_bases *= 2;
            }
            if (enemy_bases) {
                factions_present++;
                plr.region_flags[region] |= enemy_id != faction_id ? REGION_UNK_20 : REGION_UNK_10;
                if (enemy_status & DIPLO_VENDETTA) {
                    plr.region_flags[region] |= REGION_UNK_40;
                }
            } else if (!enemy_units || my_force > 5 * enemy_force) {
                continue;
            }
            if (faction_id == enemy_id || (enemy_status & DIPLO_PACT) || !(enemy_status & DIPLO_COMMLINK)) {
                continue;
            }

            bool contact_land = false;
            bool contact_any = false;
            bool estimated = false;
            if (*CurrentTurn > 1) {
                int tile_count = Continents[region].tile_count;
                if (tile_count >= 500 || my_bases * enemy.region_total_bases[region] >= 50) {
                    int period = tile_count >= 1000
                        ? ((*CurrentTurn + 2 * faction_id) & 0xF)
                        : ((faction_id + *CurrentTurn) & 7);
                    if (period) {
                        contact_land = contact_any = !(enemy_bases && plr.region_base_plan[region] > PLAN_DEFENSE);
                        estimated = true;
                    }
                }
            }
            if (!estimated) {
                if (!enemy_bases || contiguous(faction_id, enemy_id, region, 0)) {
                    contact_land = true;
                    contact_any = true;
                } else {
                    contact_land = false;
                    contact_any = contiguous(faction_id, enemy_id, region, 1) != 0;
                }
            }

            if (enemy_status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800)) {
                if (contact_any) {
                    contact_count++;
                }
                if (contact_land) {
                    if (++land_contact_count > 1) {
                        plr.region_flags[region] |= REGION_UNK_200;
                    }
                }
                if ((my_force <= enemy_units || my_force <= enemy_force)
                && my_units <= enemy_force && my_bases && contact_any) {
                    threat_count++;
                } else {
                    weak_count++;
                }
                if (enemy_force >= 3 * my_force / 2 && my_bases > enemy_bases && contact_any) {
                    plr.region_flags[region] |= REGION_UNK_4000;
                }
                if (is_human(enemy_id) && (contact_land || contact_any)) {
                    for (int i = 0; i < *BaseCount; i++) {
                        if (Bases[i].faction_id == enemy_id
                        && Bases[i].faction_id_former == faction_id
                        && mapsq(Bases[i].x, Bases[i].y)->region == region) {
                            plr.region_flags[region] |= REGION_UNK_400;
                            break;
                        }
                    }
                }
            } else if (my_bases && (enemy_status & DIPLO_WANT_REVENGE) && enemy_force > my_units) {
                if ((my_force > enemy_units && enemy_bases && enemy_force < 2 * my_force) || !contact_any) {
                    plan_flags |= 2;
                } else {
                    plan_flags |= 1;
                }
            }

            if (!(enemy_status & (DIPLO_PACT|DIPLO_TREATY))
            || ((mod_wants_to_attack(faction_id, enemy_id, 0) || mod_wants_to_attack(enemy_id, faction_id, 0))
            && contact_any)) {
                conflict = true;
            }
        }

        int unk_6 = plr.region_unk_6[region];
        int satan_bonus = great_satan(FactionRankings[7], 0);
        int growth_base = (plr.base_count > 4) + (unk_6 < 4) - ((*GameState & STATE_UNK_100) ? 2 : 0)
            + satan_bonus + factions_present + (*MapAreaTiles > 3200);
        int expand = 0;
        int growth_limit = (*CurrentTurn > 75) + growth_base + 3;
        if (my_bases) {
            int expand_flags;
            if (unk_6 >= 2 || !plr.region_total_bases[region]) {
                expand_flags = 0;
            } else {
                expand_flags = 1;
            }
            int good_tiles = plr.region_good_tiles[region];
            int free_tiles = good_tiles - plr.region_unk_5[region];
            expand_flags |= (free_tiles < 4) | (my_bases >= 20) | (plr.region_flags[region] & REGION_UNK_8);
            expand = (2 * my_bases * (growth_limit + plr.AI_growth - plr.AI_wealth)
                > good_tiles + Factions[0].region_good_tiles[region]
                && unk_6 + free_tiles / ((my_bases + 1) / 2) < 2 * growth_limit) | expand_flags;
        }
        if (conflict) {
            plr.region_flags[region] |= REGION_UNK_4;
        }
        if (expand) {
            plr.region_flags[region] |= REGION_UNK_1;
        }
        plr.region_base_plan[region] = expand ? PLAN_NAVAL_TRANSPORT : PLAN_COLONY;
        if (contact_count == 0 && plr.region_total_bases[region]) {
            plr.region_flags[region] |= REGION_UNK_100;
        } else {
            if (weak_count) {
                plan_flags = 0;
                plr.region_base_plan[region] = PLAN_OFFENSE;
            } else if (plan_flags & 2) {
                plr.region_base_plan[region] = PLAN_OFFENSE;
            }
            if (threat_count || (plan_flags & 1)) {
                plr.region_base_plan[region] = PLAN_DEFENSE;
            }
        }
        if (region >= MaxRegionLandNum) {
            plr.region_base_plan[region] = PLAN_NAVAL_TRANSPORT;
            continue;
        }
        if (!skip_targeting && region == target_region && land_contact_count >= 2) {
            for (int i = 0; i < *VehCount; i++) {
                if (Vehs[i].faction_id == faction_id && (Vehs[i].state & VSTATE_UNK_40000)) {
                    Vehs[i].state |= VSTATE_UNK_20000;
                }
            }
            plr.target_y = -1;
            plr.target_x = -1;
            plr.unk_72 = 0;
            plr.unk_71 = 0;
            target_region = -1;
        }
        if (plr.target_x < 0 && !skip_targeting && !is_human(faction_id)) {
            uint8_t region_plan = plr.region_base_plan[region];
            if (region_plan >= PLAN_DEFENSE || land_contact_count >= 2
            || (plr.region_flags[region] & REGION_UNK_4000)) {
                if (region_plan == PLAN_NAVAL_TRANSPORT && plr.region_total_bases[region] > best_value) {
                    best_value = plr.region_total_bases[region];
                    best_region = region;
                }
            } else if (plr.region_total_bases[region] + 1000 > best_value) {
                best_value = plr.region_total_bases[region] + 1000;
                best_region = region;
                int attack_base = plr.base_id_attack_target;
                if (attack_base >= 0
                && (plr.diplo_status[Bases[attack_base].faction_id] & DIPLO_VENDETTA)
                && mapsq(Bases[attack_base].x, Bases[attack_base].y)->region == region) {
                    best_value = 0;
                }
            }
        }

        if (!is_human(faction_id)
        || (faction_id == MapWin->cOwner && (*GamePreferences & PREF_AUTO_FORMER_RAISE_LWR_TERRAIN))) {
            if (plr.region_total_bases[region]) {
                uint8_t region_plan = plr.region_base_plan[region];
                if (region_plan == PLAN_NAVAL_TRANSPORT
                || (region_plan == PLAN_COLONY && Continents[region].tile_count < 150)) {
                    int best_score = 0;
                    int best_x = 0;
                    int best_y = 0;
                    for (int ay = 0; ay < *MapAbstractY; ay++) {
                        int tile_y = ay * 5;
                        for (int ax = ay & 1; ax < *MapAbstractX; ax += 2) {
                            int tile_x = ax * 5;
                            if ((uint8_t)abstract_at(ax, ay) != region) {
                                continue;
                            }
                            int cell_score = 0;
                            int best_dir = -1;
                            int cand_x = 0;
                            int cand_y = 0;
                            for (int dir = 0; dir < 8; dir++) {
                                int nx = BaseOffsetX[dir] + ax;
                                if (!*MapToggleFlat) {
                                    if (nx >= 0) {
                                        if (nx >= *MapAbstractX) {
                                            nx -= *MapAbstractX;
                                        }
                                    } else {
                                        nx += *MapAbstractX;
                                    }
                                }
                                int ny = BaseOffsetY[dir] + ay;
                                if (nx < 0 || nx >= *MapAbstractX || ny < 0 || ny >= *MapAbstractY) {
                                    continue;
                                }
                                int neighbor = (uint8_t)abstract_at(nx, ny);
                                if (neighbor == region) {
                                    cell_score--;
                                    continue;
                                }
                                cell_score += 2;
                                int tx = 0;
                                int ty = 0;
                                bool reached = false;
                                for (int j = 0; j < 8; j++) {
                                    tx = wrap(tile_x + BaseOffsetX[j]);
                                    ty = tile_y + BaseOffsetY[j];
                                    if (!on_map(tx, ty) || is_ocean(mapsq(tx, ty))) {
                                        continue;
                                    }
                                    int steps = 1;
                                    reached = true;
                                    while (true) {
                                        int wx = wrap(tx + BaseOffsetX[dir]);
                                        int wy = ty + BaseOffsetY[dir];
                                        if (!on_map(wx, wy) || is_ocean(mapsq(wx, wy))) {
                                            break;
                                        }
                                        tx = wx;
                                        ty = wy;
                                        if (++steps >= 100) {
                                            reached = false;
                                            break;
                                        }
                                    }
                                    if (reached) {
                                        break;
                                    }
                                }
                                if (!reached) {
                                    continue;
                                }
                                if (mapsq(tx, ty)->base_who() < 0) {
                                    best_dir = dir;
                                    cand_x = tx;
                                    cand_y = ty;
                                    cell_score += 4;
                                    if (neighbor && Continents[neighbor].tile_count >= 40) {
                                        cell_score += 2;
                                    }
                                }
                            }
                            if (cell_score > best_score && best_dir >= 0) {
                                best_score = cell_score;
                                best_x = cand_x;
                                best_y = cand_y;
                            }
                        }
                    }
                    if (best_score > 0) {
                        if (terraform_cost(best_x, best_y, faction_id) <= plr.energy_credits / 8) {
                            add_goal(faction_id, AI_GOAL_UNK_4, 2, best_x, best_y, -1);
                        }
                    }
                }
            }
        }
        if (!is_human(faction_id)) {
            mod_bases_reset(region, faction_id, plr.region_base_plan[region] == old_plan);
        }
    }

    if (best_region > 0 && best_region < 63 && !skip_targeting && !is_human(faction_id)
    && !(plr.player_flags & PFLAG_STRAT_SEARCH_OBJECTIVES)) {
        int best_rival_value = -1;
        int rival_id = -1;
        for (int enemy_id = 1; enemy_id < 8; enemy_id++) {
            if (faction_id != enemy_id) {
                uint32_t status = plr.diplo_status[enemy_id];
                int enemy_bases = Factions[enemy_id].region_total_bases[best_region];
                int vendetta_bonus = (status & DIPLO_VENDETTA) ? 2048 : 0;
                if ((status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800|DIPLO_TRUCE))
                && ((status & (DIPLO_UNK_800|DIPLO_VENDETTA)) || (waiting_units <= 3 && enemy_bases))
                && enemy_bases + vendetta_bonus > best_rival_value) {
                    best_rival_value = enemy_bases + vendetta_bonus;
                    rival_id = enemy_id;
                }
            }
        }
        if (rival_id > 0) {
            bool coast_only = best_rival_value == 2048;
            int best_dist = 9999;
            int best_base = -1;
            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                BASE& base = Bases[base_id];
                if (base.faction_id != faction_id) {
                    continue;
                }
                MAP* sq = mapsq(base.x, base.y);
                if (is_ocean(sq) || sq->region != best_region
                || (coast_only && !is_coast(base.x, base.y, 0))) {
                    continue;
                }
                int dist;
                int attack_base = plr.base_id_attack_target;
                if (attack_base >= 0
                && (plr.diplo_status[Bases[attack_base].faction_id] & (DIPLO_UNK_800|DIPLO_VENDETTA))
                && mapsq(Bases[attack_base].x, Bases[attack_base].y)->region == best_region) {
                    dist = vector_dist(base.x, base.y, Bases[attack_base].x, Bases[attack_base].y);
                    *BaseFindDist = dist;
                } else if (base_find_3(base.x, base.y, rival_id, best_region, -1, -1) >= 0) {
                    dist = *BaseFindDist;
                } else {
                    base_find_2(base.x, base.y, rival_id);
                    dist = 2 * *BaseFindDist;
                    *BaseFindDist *= 2;
                }
                if (dist < best_dist) {
                    best_dist = dist;
                    best_base = base_id;
                }
            }
            if (best_base >= 0) {
                plr.target_x = Bases[best_base].x;
                plr.target_y = Bases[best_base].y;
                if (coast_only) {
                    plr.player_flags |= PFLAG_UNK_800;
                } else {
                    plr.player_flags &= ~PFLAG_UNK_800;
                }
                for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
                    VEH& veh = Vehs[veh_id];
                    int unit_id = veh.unit_id;
                    if (veh.faction_id == faction_id
                    && !(veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000))
                    && (uint8_t)Units[unit_id].plan <= PLAN_DEFENSE
                    && Units[unit_id].triad() == TRIAD_LAND
                    && !has_abil(unit_id, ABL_DROP_POD)
                    && mapsq(veh.x, veh.y)->region == best_region
                    && (veh.order == ORDER_NONE || veh.order == ORDER_MOVE_TO)) {
                        base_find_2(veh.x, veh.y, rival_id);
                        if (*BaseFindDist > vector_dist(veh.x, veh.y, plr.target_x, plr.target_y)) {
                            int wpn = weap_strat(Units[unit_id].weapon_id, faction_id);
                            int arm = arm_strat(Units[unit_id].armor_id, faction_id);
                            if (wpn >= arm || (!(plr.unk_72 & 1) && !coast_only)) {
                                veh.state = (veh.state & ~(VSTATE_UNK_40000|VSTATE_UNK_20000)) | VSTATE_UNK_40000;
                                plr.unk_71++;
                                if (wpn < arm) {
                                    plr.unk_72 |= 1;
                                }
                                if (plr.unk_71 >= 8 || (coast_only && plr.unk_71 >= 2 * plr.unk_70 / 0x400000)) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!*CurrentTurn || !faction_id) {
        return;
    }
    memset(goal_assigned, 0, sizeof(goal_assigned));
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        if (Vehs[veh_id].faction_id == faction_id && Vehs[veh_id].status_icon == '1') {
            Vehs[veh_id].status_icon = '2';
        }
    }
    for (int pass = 0; pass < 2; pass++) {
        for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
            if (veh_id >= *VehCount) {
                continue;
            }
            VEH& veh = Vehs[veh_id];
            if (veh.faction_id != faction_id || (!pass && veh.status_icon != '2')) {
                continue;
            }
            int veh_x = veh.x;
            int veh_y = veh.y;
            MAP* veh_sq = mapsq(veh_x, veh_y);
            assert(veh_sq);
            int veh_base_who = (veh_sq ? veh_sq->base_who() : -1);
            int region = (veh_sq ? veh_sq->region : 0);
            int plan = (uint8_t)veh.plan();
            if (pass && veh.status_icon == '1') {
                if (plan == PLAN_TERRAFORM) {
                    continue;
                }
                if (plr.region_base_plan[region] <= PLAN_DEFENSE && plan <= PLAN_DEFENSE) {
                    continue;
                }
            }
            veh.state &= ~VSTATE_UNK_40000000;
            if (veh.order >= ORDER_FARM && veh.order < ORDER_MOVE_TO) {
                continue;
            }
            if (is_human(faction_id)) {
                if (!(veh.state & VSTATE_ON_ALERT)
                || (veh.plan() == PLAN_TERRAFORM && veh.order_auto_type != ORDERA_TERRA_AUTO_FULL)
                || (veh.state & (VSTATE_UNK_1000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_1000000|VSTATE_ON_ALERT)
                || ((veh.state & (VSTATE_UNK_2000000|VSTATE_ON_ALERT)) == (VSTATE_UNK_2000000|VSTATE_ON_ALERT)
                && veh.patrol_current_point < veh.waypoint_count)) {
                    continue;
                }
            }
            if (veh.state & VSTATE_UNK_10000000) {
                plr.region_total_offensive_units[region]++;
                plan = PLAN_DEFENSE;
            } else if (region < MaxRegionLandNum) {
                int divisor = 2 - plr.AI_fight;
                uint8_t region_plan = plr.region_base_plan[region];
                if (region_plan == PLAN_COLONY) {
                    divisor += 2;
                } else if (region_plan == PLAN_DEFENSE) {
                    divisor = 1;
                }
                if (plr.player_flags & PFLAG_STRAT_DEF_OBJECTIVES) {
                    divisor = 1;
                }
                int max_defenders = (plr.region_total_bases[region] + divisor - 1) / divisor;
                if (max_defenders && plan <= 1) {
                    if (region_plan > PLAN_DEFENSE
                    || (veh_base_who == faction_id
                    && (region_plan == PLAN_DEFENSE || (plr.region_flags[region] & REGION_UNK_4000))
                    && stack_check(veh_id, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, -1) < 1)) {
                        if (!(veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000))) {
                            uint8_t offensive = plr.region_total_offensive_units[region];
                            if (offensive < max_defenders) {
                                plan = PLAN_DEFENSE;
                                plr.region_total_offensive_units[region] = offensive + 1;
                                veh.state |= VSTATE_UNK_10000000;
                            }
                        }
                    }
                }
            }
            int triad = veh.triad();
            if (triad != TRIAD_AIR) {
                bool on_water = is_ocean(veh_sq);
                bool sea_unit = triad == TRIAD_SEA;
                if (on_water != sea_unit) {
                    if (veh_base_who < 0) {
                        continue;
                    }
                    for (int i = 0; i < 8; i++) {
                        int nx = wrap(veh_x + BaseOffsetX[i]);
                        int ny = veh_y + BaseOffsetY[i];
                        MAP* nsq = mapsq(nx, ny);
                        // Fix: restore region adjustment that was always skipped because original compared
                        // on_water == sea_unit when it should check is_ocean(nsq) == sea_unit
                        if (nsq && is_ocean(nsq) == sea_unit) {
                            region = nsq->region;
                            break;
                        }
                    }
                }
            }
            if (plan == PLAN_COMBAT) {
                uint8_t region_plan = plr.region_base_plan[region];
                plan = (region_plan == PLAN_DEFENSE || region_plan == PLAN_COLONY) ? PLAN_DEFENSE : PLAN_OFFENSE;
            }
            if (plr.target_x >= 0 && (veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000)) == VSTATE_UNK_40000) {
                veh.order = ORDER_MOVE_TO;
                veh.status_icon = '0';
                veh.waypoint_x[0] = plr.target_x;
                veh.waypoint_y[0] = plr.target_y;
                if (veh.plan() == PLAN_COLONY) {
                    veh.movement_turns = 0;
                }
                continue;
            }
            if (plan == PLAN_NAVAL_TRANSPORT) {
                int loaded = stack_check(veh_id, 8, -1, -1, -1);
                if (loaded < veh_cargo(veh_id)) {
                    continue;
                }
            }
            if (veh.status_icon == 'F' && plan == PLAN_DEFENSE) {
                continue;
            }
            bool reassign = false;
            if ((veh.state & (VSTATE_UNK_40000|VSTATE_UNK_20000)) == (VSTATE_UNK_40000|VSTATE_UNK_20000)) {
                plan = PLAN_OFFENSE;
                reassign = true;
            }
            bool wants_goal = true;
            if (veh.order == ORDER_HOLD && !reassign && veh_base_who >= 0) {
                int artifact_count = stack_check(veh_id, 2, PLAN_ARTIFACT, -1, -1);
                wants_goal = false;
                if (stack_check(veh_id, 1, -1, -1, -1) - artifact_count > 1) {
                    if (plan != PLAN_DEFENSE && plan != PLAN_RECON && plan != PLAN_COMBAT) {
                        wants_goal = true;
                    } else {
                        int stack_base_id = base_at(veh_x, veh_y);
                        int guards = stack_check(veh_id, 16, -1, -1, -1);
                        wants_goal = guards >= garrison_check(stack_base_id);
                    }
                }
            }
            if (!wants_goal) {
                continue;
            }
            if (plan == PLAN_TERRAFORM && (veh.flags & VFLAG_UNK_800)) {
                continue;
            }
            int best_cost = 99999;
            int best_goal = -1;
            for (int g = 0; g < 75; g++) {
                Goal& goal = plr.goals[g];
                if (goal.priority <= 0) {
                    continue;
                }
                int goal_type = goal.type;
                if (goal_type < 0) {
                    continue;
                }
                if ((goal_type & 0xF) != plan
                && (reassign || goal_type != AI_GOAL_SCOUT || plan > PLAN_RECON || veh.order == ORDER_HOLD)) {
                    continue;
                }
                if (!on_map(goal.x, goal.y)) {
                    goal.x = -1;
                    goal.y = -1;
                    continue;
                }
                MAP* goal_sq = mapsq(goal.x, goal.y);
                if (region != goal_sq->region) {
                    continue;
                }
                int unit_id = veh.unit_id;
                int goal_triad = Units[unit_id].triad();
                if (goal_triad == TRIAD_AIR
                || is_ocean(goal_sq) != (goal_triad == TRIAD_SEA)
                || Units[unit_id].group_id == 21) {
                    continue;
                }
                if (plan == PLAN_OFFENSE) {
                    if ((goal_sq->items & BIT_MONOLITH)
                    && (!want_monolith(veh_id) || reassign || goal_assigned[g])) {
                        continue;
                    }
                } else if (plan == PLAN_COLONY) {
                    if (base_territory(faction_id, goal.x, goal.y) >= 0) {
                        del_site(faction_id, AI_GOAL_COLONIZE, goal.x, goal.y, 0);
                        continue;
                    }
                } else if (plan == PLAN_RECON && has_abil(unit_id, ABL_EMPATH)) {
                    continue;
                }
                int goal_base_id = base_at(goal.x, goal.y);
                int cost = vector_dist(veh_x, veh_y, goal.x, goal.y) << 8;
                int assigned;
                if (reassign
                || (goal_base_id >= 0 && Bases[goal_base_id].faction_id == faction_id
                && Bases[goal_base_id].ai_plan_status)) {
                    assigned = 0;
                } else {
                    assigned = goal_assigned[g];
                }
                int priority = goal.priority - assigned;
                if (priority <= 0) {
                    continue;
                }
                if (reassign && goal_sq->base_who() < 0) {
                    continue;
                }
                if (goal_type == AI_GOAL_SCOUT && plan != PLAN_RECON) {
                    cost *= 2;
                    if ((veh.state & VSTATE_UNK_10000000) || goal_sq->base_who() < 0) {
                        continue;
                    }
                }
                if (assigned && (goal_type == AI_GOAL_SCOUT || goal_type == AI_GOAL_COLONIZE)) {
                    continue;
                }
                if (!(cost <= priority * Units[unit_id].speed() * (6 / (assigned + 1)) - assigned + 4
                || plr.region_base_plan[region] == plan
                || goal.type == AI_GOAL_SCOUT
                || (!assigned && goal.type == AI_GOAL_TERRAFORM_LAND))) {
                    continue;
                }
                if (goal.type == AI_GOAL_COLONIZE) {
                    int site;
                    if (goal_sq->items & BIT_BASE_RADIUS) {
                        site = 1;
                    } else {
                        site = goal_sq->val2 >> 4;
                        if (!site) {
                            site = world_site(goal.x, goal.y, 0);
                            site_set(goal.x, goal.y, site);
                        }
                        site = clamp(site, 1, 15);
                    }
                    priority *= site;
                }
                if (veh.state & VSTATE_UNK_10000000) {
                    int defender_stack = veh_at(goal.x, goal.y);
                    if (defender_stack >= 0) {
                        int designated = stack_check(defender_stack, 11, VSTATE_UNK_10000000, VSTATE_UNK_10000000, faction_id);
                        if (designated > (cost == 0)) {
                            continue;
                        }
                    }
                }
                if (goal.type == AI_GOAL_TERRAFORM_LAND) {
                    int goal_base = goal.base_id;
                    bool contested = goal_base >= 0 && (Bases[goal_base].state_flags & BSTATE_UNK_20);
                    if (contested && veh.home_base_id != goal_base) {
                        cost *= 3;
                    }
                    bool closer_former = false;
                    for (int i = 0; i < *VehCount; i++) {
                        VEH& other = Vehs[i];
                        if (other.faction_id == faction_id
                        && Units[other.unit_id].plan == PLAN_TERRAFORM
                        && i != veh_id) {
                            int other_cost = vector_dist(other.x, other.y, goal.x, goal.y) << 8;
                            if (contested && other.home_base_id != goal_base) {
                                other_cost *= 3;
                            }
                            if (other_cost < cost) {
                                closer_former = true;
                                break;
                            }
                        }
                    }
                    if (closer_former) {
                        continue;
                    }
                    if (goal_assigned[g] && (goal_base < 0 || veh.home_base_id != goal_base)) {
                        continue;
                    }
                }
                int owner = whose_territory(faction_id, goal.x, goal.y, 0, 0);
                int total_cost = cost;
                if (owner >= 0 && owner != faction_id) {
                    uint32_t owner_status = plr.diplo_status[owner];
                    if (!(owner_status & (DIPLO_VENDETTA|DIPLO_SHALL_BETRAY|DIPLO_UNK_800))) {
                        if (owner_status & DIPLO_UNK_100000) {
                            continue;
                        }
                        if (!(owner_status & DIPLO_PACT)) {
                            if (owner_status & DIPLO_TREATY) {
                                continue;
                            }
                            total_cost = 2 * cost;
                        }
                    }
                }
                if (goal.type == AI_GOAL_PREV_DEFEND) {
                    if (!has_abil(veh.unit_id, ABL_ARTILLERY) || veh.triad() != TRIAD_LAND) {
                        continue;
                    }
                    total_cost /= 2;
                }
                if (total_cost / priority < best_cost) {
                    best_cost = total_cost / priority;
                    best_goal = g;
                }
            }
            if (best_goal >= 0) {
                Goal& goal = plr.goals[best_goal];
                if (veh.status_icon < '3' || veh.status_icon > '5') {
                    goal_assigned[best_goal]++;
                }
                veh.order = ORDER_MOVE_TO;
                veh.status_icon = '1';
                veh.waypoint_x[0] = goal.x;
                veh.waypoint_y[0] = goal.y;
                if (veh.plan() == PLAN_COLONY) {
                    veh.movement_turns = 0;
                }
                if (goal.type == AI_GOAL_SCOUT) {
                    veh.state |= VSTATE_UNK_40000000;
                }
                if (goal.type == AI_GOAL_DEFEND) {
                    int base_id = base_at(goal.x, goal.y);
                    if (base_id >= 0 && Bases[base_id].faction_id == faction_id) {
                        Bases[base_id].state_flags |= BSTATE_UNK_8000000;
                        if (!is_human(faction_id)) {
                            mod_base_reset(base_id, 0);
                        }
                    }
                }
            }
        }
    }
    wipe_goals(faction_id);
}

namespace ai {

} // namespace ai
} // namespace engine
} // namespace alpha
