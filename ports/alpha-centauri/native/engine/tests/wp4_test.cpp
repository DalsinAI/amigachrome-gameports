/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP4 combat checks, driven by test_engine_wp4.py (host ASan/UBSan and
 * m68k-linux under qemu). Synthetic rules and worlds only, no game assets.
 * Where the existing pure combat modules cover a case they are used as
 * differential oracles (wp4_oracles.h); results must match exactly.
 *
 *   wp4_test unit   prints "<n> checks passed" and the oracle case counts.
 */
#include "../combat.h"
#include "wp4_oracles.h"

#include <memory>

using namespace alpha::engine;

static int checks = 0;
static int failures = 0;
static int oracle_cases = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    checks++; \
    long long va_ = (long long)(a), vb_ = (long long)(b); \
    if (va_ != vb_) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK_EQ failed: %s = %lld, %s = %lld\n", \
            __FILE__, __LINE__, #a, va_, #b, vb_); \
    } \
} while (0)

/* ---- Synthetic rules (values in the range of alpha.txt, not copied) ---- */
static char empty_str[] = "";
static char morale_names[7][16] = {"Very Green", "Green", "Disciplined", "Hardened", "Veteran", "Commando", "Elite"};

static void set_rules() {
    memset(Rules, 0, sizeof(*Rules));
    Rules->move_rate_roads = 3;
    Rules->artillery_max_rng = 2;
    Rules->artillery_dmg_numerator = 3;
    Rules->artillery_dmg_denominator = 2;
    Rules->max_dmg_percent_arty_base_bunker = 50;
    Rules->max_dmg_percent_arty_open = 99;
    Rules->max_dmg_percent_arty_sea = 100;
    Rules->max_airdrop_rng_wo_orbital_insert = 8;
    for (int i = 0; i < 3; i++) {
        Rules->psi_combat_ratio_atk[i] = 3;
        Rules->psi_combat_ratio_def[i] = 2;
    }
    const struct { int speed, triad, range, missile; } chs[] = {
        {1, TRIAD_LAND, 0, 0}, {2, TRIAD_LAND, 0, 0}, {3, TRIAD_LAND, 0, 0},
        {4, TRIAD_SEA, 0, 0}, {6, TRIAD_SEA, 0, 0}, {8, TRIAD_AIR, 2, 0},
        {8, TRIAD_AIR, 1, 0}, {8, TRIAD_AIR, 0, 0}, {12, TRIAD_AIR, 1, 1},
    };
    for (int i = 0; i < 9; i++) {
        memset(&Chassis[i], 0, sizeof(Chassis[i]));
        Chassis[i].speed = chs[i].speed;
        Chassis[i].triad = chs[i].triad;
        Chassis[i].range = chs[i].range;
        Chassis[i].missile = chs[i].missile;
    }
    const int off[] = {1, 2, 4, 5, 6, 8, 10, 12, 13, 16, 20, 24, 6, 12, 30, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const int mode[] = {0, 1, 0, 1, 2, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0,
        WMODE_COLONY, WMODE_TERRAFORM, WMODE_TRANSPORT, WMODE_SUPPLY, WMODE_PROBE, WMODE_ARTIFACT, 2, 2, 2};
    for (int i = 0; i < 26; i++) {
        memset(&Weapon[i], 0, sizeof(Weapon[i]));
        Weapon[i].name = empty_str;
        Weapon[i].offense_value = off[i];
        Weapon[i].mode = mode[i];
    }
    const int def[] = {1, 2, 3, 4, 5, 6, 8, 10, 12, -1, 3, 3, 8, 8};
    for (int i = 0; i < 14; i++) {
        memset(&Armor[i], 0, sizeof(Armor[i]));
        Armor[i].name = empty_str;
        Armor[i].defense_value = def[i];
    }
    for (int i = 0; i < MaxMoraleNum; i++) {
        Morale[i].name = morale_names[i];
        Morale[i].name_lifecycle = morale_names[i];
    }
}

static int make_unit(int unit_id, int chassis, int weapon, int armor, int reactor,
uint32_t abilities = 0, int plan = PLAN_OFFENSE) {
    UNIT* u = &Units[unit_id];
    memset(u, 0, sizeof(*u));
    snprintf(u->name, sizeof(u->name), "Unit%d", unit_id);
    u->chassis_id = chassis;
    u->weapon_id = weapon;
    u->armor_id = armor;
    u->reactor_id = reactor;
    u->ability_flags = abilities;
    u->plan = plan;
    u->preq_tech = TECH_None;
    u->cost = 3;
    u->unit_flags = UNIT_ACTIVE | UNIT_PROTOTYPED;
    return unit_id;
}

static std::unique_ptr<EngineState> state;

/* 20x20 land map, factions 1 and 2 (computer players) at war. */
static void setup(int width = 20, int height = 20) {
    state.reset(new EngineState());
    engine_reset(*state);
    engine_bind(state.get());
    config_vanilla(conf);
    set_rules();
    *MapAreaX = width;
    *MapAreaY = height;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = ALT_ONE_ABOVE_SEA << 5;
        (*MapTiles)[i].region = 1;
        (*MapTiles)[i].val2 = 0xF;
        (*MapTiles)[i].owner = -1;
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        Factions[i].target_x = -1;
        Factions[i].diff_level = 3;
    }
    for (int f = 1; f <= 3; f++) {
        set_alive(f, true);
        set_human(f, false);
    }
    for (int a = 0; a < MaxPlayerNum; a++) {
        for (int b = 0; b < MaxPlayerNum; b++) {
            if (a != b) {
                Factions[a].diplo_status[b] = DIPLO_COMMLINK | DIPLO_VENDETTA;
            }
        }
    }
    *CurrentTurn = 100;
    MapWin->cOwner = 7; // nobody involved: no battle rendering
    *CurrentPlayerFaction = 7;
    // Native life and basic units
    make_unit(BSC_MIND_WORMS, CHS_INFANTRY, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, 1, 0, PLAN_OFFENSE);
    make_unit(BSC_LOCUSTS_OF_CHIRON, CHS_NEEDLEJET, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, 1, 0, PLAN_OFFENSE);
    make_unit(BSC_FUNGAL_TOWER, CHS_INFANTRY, WPN_PSI_ATTACK, ARM_PSI_DEFENSE, 1, 0, PLAN_DEFENSE);
    make_unit(BSC_ALIEN_ARTIFACT, CHS_INFANTRY, WPN_ALIEN_ARTIFACT, ARM_NO_ARMOR, 1, 0, PLAN_ARTIFACT);
    make_unit(BSC_COLONY_POD, CHS_INFANTRY, WPN_COLONY_MODULE, ARM_NO_ARMOR, 1, 0, PLAN_COLONY);
}

static int unit_slot(int faction_id, int n) {
    return faction_id * MaxProtoFactionNum + n;
}

static MAP* sq(int x, int y) {
    return mapsq(x, y);
}

/* WP2 is not merged: a base record without base_first/production. */
static int add_base(int faction_id, int x, int y, int pop) {
    int base_id = mod_base_init(faction_id, x, y);
    CHECK(base_id >= 0);
    if (base_id >= 0) {
        Bases[base_id].pop_size = pop;
        memset(Bases[base_id].facilities_built, 0, sizeof(Bases[base_id].facilities_built));
    }
    return base_id;
}

/* ---- 1. psi_factor against combat_psi_factor ---- */
static void test_psi_factor() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    setup();
    int b1 = add_base(1, 2, 2, 3);
    int b2 = add_base(2, 8, 8, 3);
    const int rules_psi[] = {-25, 0, 10, 25, 50};
    for (int rp : rules_psi) {
        MFactions[1].rule_psi = rp;
        for (int proj = 0; proj < 4; proj++) {
            SecretProjects[FAC_DREAM_TWISTER - SP_ID_First] = (proj & 1) ? b1 : b2;
            SecretProjects[FAC_NEURAL_AMPLIFIER - SP_ID_First] = (proj & 2) ? b1 : b2;
            for (int v = 0; v <= 400; v += 7) {
                for (int atk = 0; atk < 2; atk++) {
                    for (int tower = 0; tower < 2; tower++) {
                        int expect = 0;
                        int e = wp4o::psi_factor(v, rp, atk, tower, proj & 2, proj & 1, &expect);
                        CHECK_EQ(e, 0);
                        CHECK_EQ(psi_factor(v, 1, atk, tower), expect);
                        oracle_cases++;
                    }
                }
            }
        }
    }
}

/* ---- 2. terrain_defense against the original defense_value ---- */
static void test_terrain_defense() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    setup();
    make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_SYNTHMETAL_ARMOR, 1);
    make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_PLASMA_STEEL_ARMOR, 1);
    make_unit(unit_slot(2, 2), CHS_SPEEDER, WPN_LASER, ARM_NO_ARMOR, 1);
    make_unit(unit_slot(2, 3), CHS_FOIL, WPN_LASER, ARM_NO_ARMOR, 1);
    int pb1 = add_base(1, 18, 18, 1);
    int pb2 = add_base(2, 16, 18, 1);
    const int tx = 6, ty = 6;
    const int climates[] = {1, 2, 3, 4, 6};
    const int val3s[] = {0, 0x40, 0x80, 0xC0};
    const uint32_t items[] = {0, BIT_FUNGUS, BIT_FOREST, BIT_FUNGUS | BIT_FOREST, BIT_ROAD};
    // defenders: faction 2 units and a faction 0 mind worm
    const int def_units[][2] = {{2, unit_slot(2, 1)}, {2, unit_slot(2, 2)}, {0, BSC_MIND_WORMS}};
    // attackers: none, land, native worm, faction 1 sea foil
    const int atk_units[][2] = {{-1, -1}, {1, unit_slot(1, 1)}, {0, BSC_MIND_WORMS}};
    int veh_air = -1;
    (void)veh_air;
    for (int d = 0; d < 3; d++) {
        for (int a = 0; a < 3; a++) {
            if (atk_units[a][0] == def_units[d][0]) {
                continue;
            }
            for (int pholus = -1; pholus <= 1; pholus++) {
                int pholus_owner = pholus < 0 ? -1 : (pholus == 0 ? 1 : 2);
                SecretProjects[FAC_PHOLUS_MUTAGEN - SP_ID_First] = pholus < 0 ? SP_Unbuilt : (pholus == 0 ? pb1 : pb2);
                for (int c : climates) {
                    for (int r : val3s) {
                        for (uint32_t it : items) {
                            *VehCount = 0;
                            for (int i = 0; i < *MapAreaTiles; i++) {
                                (*MapTiles)[i].val2 = 0xF;
                                (*MapTiles)[i].items &= BIT_BASE_IN_TILE;
                            }
                            rebuild_base_bits();
                            MAP* m = sq(tx, ty);
                            m->climate = (uint8_t)(c << 5);
                            m->val3 = (uint8_t)r;
                            m->items = it;
                            int vd = veh_init(def_units[d][1], def_units[d][0], tx, ty);
                            int va = -1;
                            if (atk_units[a][0] >= 0) {
                                va = veh_init(atk_units[a][1], atk_units[a][0], tx + 1, ty + 1);
                            }
                            int got = terrain_defense(&Vehs[vd], va >= 0 ? &Vehs[va] : nullptr);
                            int got_label = label_id_of(*VehBattleDisplayTerrain);
                            wp4o::Terrain t = {};
                            t.climate = m->climate;
                            t.val3 = m->val3;
                            t.items = m->items;
                            t.base_owner_nibble = m->val2 & 0xF;
                            t.def_faction = def_units[d][0];
                            t.def_unit = def_units[d][1];
                            t.def_weapon_offense = Units[def_units[d][1]].offense_value();
                            t.def_triad = Units[def_units[d][1]].triad();
                            t.atk_present = va >= 0;
                            t.atk_unit = va >= 0 ? atk_units[a][1] : 0;
                            t.atk_weapon_offense = va >= 0 ? Units[atk_units[a][1]].offense_value() : 0;
                            t.atk_triad = va >= 0 ? Units[atk_units[a][1]].triad() : 0;
                            t.atk_faction = va >= 0 ? atk_units[a][0] : 0;
                            t.pholus_owner = pholus_owner;
                            int mult = 0, label = 0;
                            CHECK_EQ(wp4o::terrain(t, &mult, &label), 0);
                            CHECK_EQ(got, mult);
                            (void)got_label;
                            oracle_cases++;
                        }
                    }
                }
            }
        }
    }
}

/* ---- 3. Native life cycle against the original morale_alien ---- */
static void test_native_morale() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    setup();
    const int turns[] = {0, 1, 44, 45, 89, 90, 169, 170, 249, 250, 300, 329, 330, 500};
    const int units[] = {BSC_MIND_WORMS, BSC_LOCUSTS_OF_CHIRON, BSC_FUNGAL_TOWER};
    for (int turn : turns) {
        *CurrentTurn = turn;
        for (int u : units) {
            for (int opp = -1; opp <= 2; opp++) {
                for (int extra = 0; extra < 8; extra++) {
                    *VehCount = 0;
                    for (int i = 0; i < *MapAreaTiles; i++) {
                        (*MapTiles)[i].items = 0;
                        (*MapTiles)[i].val2 = 0xF;
                    }
                    Factions[2].major_atrocities = (extra & 1);
                    TectonicDetonationCount[2] = (extra & 2) ? 1 : 0;
                    int v = veh_init(u, 0, 10, 10);
                    Vehs[v].state = (extra & 4) ? (uint32_t)VSTATE_MONOLITH_UPGRADED : 0u;
                    Vehs[v].set_lifecycle_value(extra % 4);
                    int fungus = (turn + extra) % 9;
                    for (int i = 1; i <= fungus && i < 9; i++) {
                        sq(wrap(10 + TableOffsetX[i]), 10 + TableOffsetY[i])->items |= BIT_FUNGUS;
                    }
                    int got = mod_morale_alien(v, opp);
                    int expect = -1;
                    CHECK_EQ(wp4o::native_morale(turn, u, opp, Factions[2].major_atrocities,
                        TectonicDetonationCount[2], Vehs[v].state, Vehs[v].flags, fungus, &expect), 0);
                    if (opp == 1) {
                        // the oracle reads faction 2 facts; faction 1 has none
                        Factions[2].major_atrocities = 0;
                        TectonicDetonationCount[2] = 0;
                        CHECK_EQ(wp4o::native_morale(turn, u, opp, 0, 0, Vehs[v].state,
                            Vehs[v].flags, fungus, &expect), 0);
                    }
                    if (got != expect) {
                        fprintf(stderr, "native morale turn %d unit %d opp %d extra %d: %d != %d\n",
                            turn, u, opp, extra, got, expect);
                    }
                    CHECK_EQ(got, expect);
                    oracle_cases++;
                }
            }
        }
    }
}

/* ---- 4. Basic offense/defense against the original get_basic_* ---- */
static uint32_t lcg = 12345;
static int rnd(int n) {
    lcg = lcg * 1103515245u + 12345u;
    return (int)((lcg >> 16) % (uint32_t)n);
}

static void test_basic_strengths() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    setup();
    for (int n = 0; n < 600; n++) {
        *VehCount = 0;
        *BaseCount = 0;
        for (int i = 0; i < *MapAreaTiles; i++) {
            (*MapTiles)[i].items = 0;
            (*MapTiles)[i].val2 = 0xF;
        }
        int wa = rnd(12), aa = rnd(9), wd = rnd(12), ad = rnd(9);
        uint32_t abl_a = rnd(3) == 0 ? ABL_SOPORIFIC_GAS : (rnd(4) == 0 ? ABL_DISSOCIATIVE_WAVE : 0);
        uint32_t abl_d = rnd(3) == 0 ? ABL_SOPORIFIC_GAS : (rnd(4) == 0 ? ABL_DISSOCIATIVE_WAVE : 0);
        int plan_d = rnd(6) == 0 ? (rnd(2) ? PLAN_PROBE : PLAN_ARTIFACT) : PLAN_DEFENSE;
        int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, wa, aa, 1 + rnd(4), abl_a);
        int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, plan_d == PLAN_PROBE ? WPN_PROBE_TEAM :
            (plan_d == PLAN_ARTIFACT ? WPN_ALIEN_ARTIFACT : wd), plan_d == PLAN_PROBE && rnd(2) ? ARM_NO_ARMOR : ad,
            1 + rnd(4), abl_d, plan_d);
        bool native_atk = rnd(5) == 0;
        Factions[1].SE_morale = rnd(9) - 4;
        Factions[2].SE_morale = rnd(9) - 4;
        Factions[1].SE_morale_pending = rnd(5);
        Factions[2].SE_morale_pending = rnd(5);
        bool at_base = rnd(2);
        if (at_base) {
            add_base(2, 10, 10, 2);
            add_base(1, 9, 9, 2);
        }
        int vd = veh_init(ud, 2, 10, 10);
        int va = native_atk ? veh_init(BSC_MIND_WORMS, 0, 9, 9) : veh_init(ua, 1, 9, 9);
        Vehs[va].morale = rnd(7);
        Vehs[vd].morale = rnd(7);
        int fa = Vehs[va].faction_id;
        // Normal attacker offense (unk_tgl 0) and defender defense
        int off = get_basic_offense(va, vd, 0, 0, 0);
        int off_morale = VehBasicBattleMorale[0];
        int dfn = get_basic_defense(vd, va, 0, 0);
        int dfn_morale = VehBasicBattleMorale[1];
        wp4o::Basic b = {};
        // attacker side (offense_mode false: no base bonus, not clamped)
        b.unit_id = Vehs[va].unit_id;
        b.weapon_offense = Units[b.unit_id].offense_value();
        b.armor_defense = Units[b.unit_id].defense_value();
        b.plan = Units[b.unit_id].plan;
        b.opposing_plan = Units[ud].plan;
        b.initial_morale = fa ? morale_veh(va, true, 0) : mod_morale_alien(va, 2);
        b.se_morale = Factions[fa].SE_morale;
        b.se_morale_pending = Factions[fa].SE_morale_pending;
        b.at_base = at_base;
        b.opponent_native = false;
        b.dissociative = has_abil(b.unit_id, ABL_DISSOCIATIVE_WAVE);
        b.soporific = has_abil(ud, ABL_SOPORIFIC_GAS);
        b.offense_mode = false;
        b.factored_offense = mod_offense_proto(b.unit_id, vd, 0);
        b.factored_defense = mod_armor_proto(b.unit_id, vd, 0);
        int eo = 0, ed = 0, ma = 0, md = 0;
        if (wp4o::basic(b, &eo, &ed, &ma, &md) == 0) {
            if (!native_atk && b.initial_morale >= 0) {
                CHECK_EQ(off, eo);
                CHECK_EQ(off_morale, ma);
                oracle_cases++;
            }
        }
        // defender side
        b = wp4o::Basic();
        b.unit_id = ud;
        b.weapon_offense = Units[ud].offense_value();
        b.armor_defense = Units[ud].defense_value();
        b.plan = Units[ud].plan;
        b.opposing_plan = Units[Vehs[va].unit_id].plan;
        b.initial_morale = morale_veh(vd, true, 0);
        b.se_morale = Factions[2].SE_morale;
        b.se_morale_pending = Factions[2].SE_morale_pending;
        b.at_base = at_base;
        b.opponent_native = native_atk;
        b.dissociative = has_abil(ud, ABL_DISSOCIATIVE_WAVE);
        b.soporific = !native_atk && has_abil(Vehs[va].unit_id, ABL_SOPORIFIC_GAS);
        b.offense_mode = false;
        b.factored_offense = mod_offense_proto(ud, va, 0);
        b.factored_defense = mod_armor_proto(ud, va, 0);
        if (wp4o::basic(b, &eo, &ed, &ma, &md) == 0) {
            CHECK_EQ(dfn, ed);
            if (!(plan_d == PLAN_ARTIFACT)) {
                CHECK_EQ(dfn_morale, md);
            }
            oracle_cases++;
        }
        // defender firing back in a weapon duel (unk_tgl 1, offense_mode)
        if (!native_atk) {
            int off2 = get_basic_offense(vd, va, 0, 0, 1);
            b.offense_mode = true;
            b.opponent_native = false;
            b.soporific = has_abil(Vehs[va].unit_id, ABL_SOPORIFIC_GAS);
            b.factored_offense = mod_offense_proto(ud, va, 0);
            if (wp4o::basic(b, &eo, &ed, &ma, &md) == 0) {
                CHECK_EQ(off2, eo);
                oracle_cases++;
            }
        }
    }
}

/* ---- Battle scenario helpers ---- */
static int place(int unit_id, int faction_id, int x, int y, int morale = 2) {
    int v = veh_init(unit_id, faction_id, x, y);
    Vehs[v].morale = morale;
    return v;
}

static BattleResult* new_result() {
    static BattleResult r;
    return &r;
}

/* ---- 5. Full battles against combat_rounds/combat_crt ---- */
static void test_duels() {
    for (int n = 0; n < 300; n++) {
        setup();
        int ra = 1 + rnd(4), rd = 1 + rnd(4);
        int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, rnd(12), rnd(9), ra);
        int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, rnd(12), rnd(9), rd);
        int va = place(ua, 1, 5, 5, rnd(7));
        int vd = place(ud, 2, 6, 6, rnd(7));
        Vehs[va].damage_taken = rnd(10 * ra - 1);
        Vehs[vd].damage_taken = rnd(10 * rd - 1);
        if (n % 3 == 0) {
            sq(6, 6)->val3 = 0x80; // rocky
        } else if (n % 3 == 1) {
            sq(6, 6)->items |= BIT_FOREST;
        }
        int dmg_a = Vehs[va].damage_taken, dmg_d = Vehs[vd].damage_taken;
        int count = *VehCount;
        uint32_t seed = 1000u + (uint32_t)n * 7919u;
        game_srand(seed);
        BattleResult* r = new_result();
        int outcome = engine_attack(va, 6, 6, r);
        CHECK(outcome == BATTLE_ATTACKER_WON || outcome == BATTLE_DEFENDER_WON);
        CHECK_EQ(r->veh_id_def >= 0 || outcome == BATTLE_ATTACKER_WON, 1);
        CHECK_EQ(*VehCount, count - 1);
        // The rounds use the recorded battle strengths (all modifiers applied).
        wp4o::Duel d = {r->offense, r->defense, ra, rd, dmg_a, dmg_d};
        wp4o::DuelResult o = {};
        CHECK_EQ(wp4o::rounds(d, seed, &o), 0);
        CHECK_EQ(r->rounds, o.rounds);
        CHECK_EQ(r->attacker_hits, o.attacker_hits);
        CHECK_EQ(r->defender_hits, o.defender_hits);
        CHECK_EQ(outcome == BATTLE_ATTACKER_WON, o.attacker_won);
        CHECK_EQ(r->hp_atk_after, o.attacker_won ? 10 * ra - o.damage_atk : 0);
        CHECK_EQ(r->hp_def_after, o.attacker_won ? 0 : 10 * rd - o.damage_def);
        CHECK_EQ(r->num_killed, 1);
        oracle_cases++;
    }
}

/* Same defender strength in battle_compute as in the recorded battle. */
static void test_compute_matches_record() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_GATLING_LASER, ARM_SYNTHMETAL_ARMOR, 2);
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_SILKSTEEL_ARMOR, 2);
    int va = place(ua, 1, 5, 5, 3);
    int vd = place(ud, 2, 6, 6, 4);
    int off = 0, dfn = 0;
    mod_battle_compute(va, vd, &off, &dfn, 0);
    // Infantry, open ground: offense = 5*8*(3+6)*4, defense = 4*8*(4+6)*2*2
    CHECK_EQ(off, 5 * 8 * 9 * 4);
    CHECK_EQ(dfn, 4 * 8 * 10 * 4);
    int odds = engine_attack_odds(va, 6, 6, nullptr);
    CHECK_EQ(odds, off * 8 / (dfn + 1));
    game_srand(7);
    BattleResult* r = new_result();
    engine_attack(va, 6, 6, r);
    CHECK_EQ(r->offense, off);
    CHECK_EQ(r->defense, dfn);
    CHECK_EQ(r->morale_atk, 3);
    CHECK_EQ(r->morale_def, 4);
    // Hasty: one third of the moves left reduces the offense by two thirds.
    setup();
    make_unit(ua, CHS_INFANTRY, WPN_GATLING_LASER, ARM_SYNTHMETAL_ARMOR, 2);
    make_unit(ud, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_SILKSTEEL_ARMOR, 2);
    va = place(ua, 1, 5, 5, 3);
    vd = place(ud, 2, 6, 6, 4);
    Vehs[va].moves_spent = 2;
    engine_attack(va, 6, 6, r);
    CHECK_EQ(r->offense, off * 1 / 3);
    CHECK(r->mod_count[0] >= 1);
    CHECK_EQ(r->mods[0][0].percent, -66);
}

/* ---- 6. Sensor bonus against sensor_combat_adjustment ---- */
static void test_sensor() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    for (int pct = 0; pct <= 50; pct += 25) {
        for (int cov = 0; cov < 3; cov++) {
            setup();
            Rules->combat_defend_sensor = pct;
            int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
            int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_PLASMA_STEEL_ARMOR, 1);
            int va = place(ua, 1, 5, 5);
            int vd = place(ud, 2, 6, 6);
            int off0 = 0, def0 = 0;
            mod_battle_compute(va, vd, &off0, &def0, 0);
            unsigned coverage = 0;
            if (cov >= 1) {
                sq(7, 7)->items |= BIT_SENSOR;
                coverage = 1;
            }
            if (cov == 2) {
                int b = add_base(2, 8, 8, 2);
                set_fac(FAC_GEOSYNC_SURVEY_POD, b, 1);
                sq(7, 7)->items &= ~BIT_SENSOR;
                coverage = 3;
            }
            int off1 = 0, def1 = 0;
            mod_battle_compute(va, vd, &off1, &def1, 0);
            int expect = 0;
            CHECK_EQ(wp4o::sensor(def0, coverage, pct, &expect), 0);
            CHECK_EQ(def1, expect);
            CHECK_EQ(off1, off0);
            oracle_cases++;
        }
    }
}

/* ---- 7. Difficulty handicap against combat_difficulty ---- */
static void test_difficulty() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    for (int human = 0; human < 4; human++) {
        for (int da = 0; da < 4; da++) {
            for (int dd = 0; dd < 4; dd++) {
                int offs[2];
                for (int pass = 0; pass < 2; pass++) {
                    setup();
                    set_human(1, pass && (human & 1));
                    set_human(2, pass && (human & 2));
                    Factions[1].diff_level = pass ? da : 3;
                    Factions[2].diff_level = pass ? dd : 3;
                    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_CHAOS_GUN, ARM_NO_ARMOR, 1);
                    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NEUTRONIUM_ARMOR, 4);
                    int va = place(ua, 1, 5, 5);
                    place(ud, 2, 6, 6);
                    game_srand(3);
                    BattleResult* r = new_result();
                    engine_attack(va, 6, 6, r);
                    offs[pass] = r->offense;
                }
                int expect = 0;
                unsigned mask = (human & 1 ? 2u : 0u) | (human & 2 ? 4u : 0u);
                CHECK_EQ(wp4o::difficulty(1, 2, mask, da, dd, offs[0], &expect), 0);
                CHECK_EQ(offs[1], expect);
                oracle_cases++;
            }
        }
    }
}

/* ---- 8. Artillery duel stage against artillery_combat ---- */
static void test_artillery_stage() {
    if (!wp4o::have_prebattle()) {
        return;
    }
    for (int n = 0; n < 120; n++) {
        setup();
        Rules->combat_land_vs_sea_artillery = 25 * rnd(3);
        Rules->combat_artillery_bonus_altitude = 25 * rnd(3);
        int land_a = rnd(2), land_d = rnd(2);
        int ua = make_unit(unit_slot(1, 1), land_a ? CHS_INFANTRY : CHS_FOIL, 1 + rnd(10), rnd(8), 1, land_a ? ABL_ARTILLERY : 0);
        int ud = make_unit(unit_slot(2, 1), land_d ? CHS_INFANTRY : CHS_FOIL, 1 + rnd(10), rnd(8), 1, land_d ? ABL_ARTILLERY : 0);
        int ca = 3 + rnd(4), cd = 3 + rnd(4);
        if (!land_a) {
            ca = 1;
        }
        if (!land_d) {
            cd = 1;
        }
        sq(5, 5)->climate = (uint8_t)(ca << 5);
        sq(7, 5)->climate = (uint8_t)(cd << 5);
        int va = place(ua, 1, 5, 5, rnd(7));
        int vd = place(ud, 2, 7, 5, rnd(7));
        int ct = CT_WEAPON_ONLY | (rnd(2) ? CT_CAN_ARTY : 0);
        int is_bombard = ct & CT_CAN_ARTY;
        int in_off = get_basic_offense(va, vd, 0, is_bombard, 0);
        int in_def = get_basic_offense(vd, va, 0, is_bombard, 1);
        int off = 0, dfn = 0;
        mod_battle_compute(va, vd, &off, &dfn, ct);
        wp4o::Artillery a = {in_off, in_def, Units[ua].triad(), Units[ud].triad(),
            sq(5, 5)->climate, sq(7, 5)->climate, (unsigned)ct,
            Rules->combat_land_vs_sea_artillery, Rules->combat_artillery_bonus_altitude};
        int eo = 0, ed = 0;
        CHECK_EQ(wp4o::artillery(a, &eo, &ed), 0);
        CHECK_EQ(off, eo);
        CHECK_EQ(dfn, ed);
        oracle_cases++;
    }
}

/* ---- 9. Retreat, bombardment, collateral damage, bases ---- */
static void test_retreat() {
    bool seen = false;
    for (int seed = 1; seed < 60 && !seen; seed++) {
        setup();
        int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_FUSION_LASER, ARM_NO_ARMOR, 1);
        int ud = make_unit(unit_slot(2, 1), CHS_HOVERTANK, WPN_LASER, ARM_NO_ARMOR, 1);
        int va = place(ua, 1, 5, 5);
        place(ud, 2, 6, 6);
        game_srand(seed);
        BattleResult* r = new_result();
        int out = engine_attack(va, 6, 6, r);
        if (out == BATTLE_DEFENDER_RETREATED) {
            seen = true;
            CHECK(r->retreat_x >= 0);
            CHECK_EQ(map_range(6, 6, r->retreat_x, r->retreat_y), 1);
            CHECK_EQ(veh_at(6, 6), -1);
            CHECK(veh_at(r->retreat_x, r->retreat_y) >= 0);
            // moved away from the attacker: not adjacent to (5, 5)
            CHECK(map_range(5, 5, r->retreat_x, r->retreat_y) >= 2);
            CHECK(r->hp_def_after > 0 && r->hp_def_after <= 5);
            CHECK_EQ(*VehCount, 2);
        }
    }
    CHECK(seen);
}

static void test_bombardment() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_CHAOS_GUN, ARM_NO_ARMOR, 1, ABL_ARTILLERY);
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    int va = place(ua, 1, 5, 5);
    CHECK_EQ(map_range(5, 5, 9, 5), 2);
    for (int i = 0; i < 3; i++) {
        place(ud, 2, 9, 5);
    }
    game_srand(11);
    BattleResult* r = new_result();
    int out = engine_attack(va, 9, 5, r); // range 2
    CHECK_EQ(out, BATTLE_ARTILLERY);
    CHECK_EQ(r->combat_type, CT_CAN_ARTY);
    int shots = 0;
    for (int i = 0; i < r->event_count; i++) {
        if (r->events[i].type == BE_ARTILLERY) {
            shots++;
        }
    }
    CHECK_EQ(shots, 3);
    CHECK_EQ(*VehCount, 4); // open ground: at most 99% damage, nobody dies
    for (int v = veh_at(9, 5); v >= 0; v = Vehs[v].next_veh_id_stack) {
        CHECK(Vehs[v].cur_hitpoints() >= 1);
    }
    CHECK_EQ(Vehs[va].damage_taken, 0);
    CHECK_EQ(Vehs[va].x, 5);
    // Out of range and non-artillery units cannot bombard.
    int ub = make_unit(unit_slot(1, 2), CHS_INFANTRY, WPN_CHAOS_GUN, ARM_NO_ARMOR, 1);
    int vb = place(ub, 1, 5, 5);
    CHECK_EQ(engine_attack(vb, 9, 5, r), BATTLE_NO_TARGET);
    place(ud, 2, 13, 5);
    CHECK_EQ(engine_attack(va, 13, 5, r), BATTLE_NO_TARGET); // range 4 > artillery_max_rng
}

static void test_collateral_and_base() {
    // Open ground: winner kills the defender, the rest of the stack takes
    // collateral damage of 3 per attacker reactor level (reactor 4 would
    // kill the whole stack of reactor 1 units).
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_SINGULARITY_LASER, ARM_NO_ARMOR, 1);
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    int va = place(ua, 1, 5, 5, 6);
    place(ud, 2, 6, 6);
    place(ud, 2, 6, 6);
    place(ud, 2, 6, 6);
    game_srand(5);
    BattleResult* r = new_result();
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_ATTACKER_WON);
    CHECK_EQ(r->num_killed, 1);
    CHECK_EQ(r->num_damaged, 2);
    CHECK_EQ(*VehCount, 3);
    for (int v = veh_at(6, 6); v >= 0; v = Vehs[v].next_veh_id_stack) {
        CHECK_EQ(Vehs[v].damage_taken, 3 * 1);
    }
    CHECK(r->veh_id_atk >= 0 && Vehs[r->veh_id_atk].unit_id == ua);

    // A base: only the defender dies; the base loses one population when
    // its last defender is killed.
    setup();
    make_unit(ua, CHS_INFANTRY, WPN_SINGULARITY_LASER, ARM_NO_ARMOR, 4);
    make_unit(ud, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    int b = add_base(2, 6, 6, 3);
    va = place(ua, 1, 5, 5, 6);
    place(ud, 2, 6, 6);
    place(ud, 2, 6, 6);
    game_srand(5);
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_ATTACKER_WON);
    CHECK_EQ(r->num_killed, 1);
    CHECK_EQ(r->num_damaged, 0);
    CHECK_EQ(Bases[b].pop_size, 3);
    CHECK(Bases[b].state_flags & BSTATE_COMBAT_LOSS_LAST_TURN);
    CHECK_EQ(stack_check(veh_at(6, 6), 1, -1, -1, -1), 1);
    va = r->veh_id_atk;
    Vehs[va].moves_spent = 0;
    Vehs[va].damage_taken = 0;
    game_srand(6);
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_ATTACKER_WON);
    CHECK_EQ(Bases[b].pop_size, 2);
    CHECK_EQ(Bases[b].factions_pop_size_intel[1], 2);

    // Last defender of a size 1 base: the base is destroyed.
    setup();
    make_unit(ua, CHS_INFANTRY, WPN_SINGULARITY_LASER, ARM_NO_ARMOR, 4);
    make_unit(ud, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    add_base(2, 12, 12, 3);
    b = add_base(2, 6, 6, 1);
    va = place(ua, 1, 5, 5, 6);
    place(ud, 2, 6, 6);
    game_srand(5);
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_ATTACKER_WON);
    CHECK(r->base_destroyed);
    CHECK_EQ(*BaseCount, 1);
    CHECK_EQ(base_at(6, 6), -1);
}

static size_t not_implemented_count(const char* name) {
    size_t n = 0;
    for (const std::string& line : headless_sink()->log) {
        if (line.find(name) != std::string::npos) {
            n++;
        }
    }
    return n;
}

static void test_capture() {
    setup();
    headless_sink()->clear();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int b = add_base(2, 6, 6, 2);
    int va = place(ua, 1, 5, 5);
    BattleResult* r = new_result();
    size_t before = not_implemented_count("mod_capture_base");
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_BASE_CAPTURED);
    CHECK(r->base_captured);
    CHECK_EQ(r->base_id, b);
    CHECK_EQ(Vehs[va].x, 6);
    CHECK_EQ(Vehs[va].y, 6);
    CHECK_EQ(Vehs[va].moves_spent, Rules->move_rate_roads);
    // Integration: WP2's mod_capture_base is linked now (was a stub): the base changed hands.
    CHECK_EQ(not_implemented_count("mod_capture_base"), before);
    CHECK(r->base_id >= 0 && r->base_id < *BaseCount && Bases[r->base_id].faction_id == 1);
    // Non-combat units and sea units cannot capture.
    setup();
    b = add_base(2, 6, 6, 2);
    int vc = place(BSC_COLONY_POD, 1, 5, 5);
    CHECK_EQ(engine_attack(vc, 6, 6, r), BATTLE_NO_TARGET);
    CHECK_EQ(Vehs[vc].x, 5);
}

static void test_best_defender() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int weak = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    int strong = make_unit(unit_slot(2, 2), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_SILKSTEEL_ARMOR, 1);
    int va = place(ua, 1, 5, 5);
    place(weak, 2, 6, 6);
    int vs = place(strong, 2, 6, 6);
    place(weak, 2, 6, 6);
    CHECK_EQ(mod_best_defender(veh_at(6, 6), va, 0), vs);
    // A damaged strong unit loses its place.
    Vehs[vs].damage_taken = 9;
    CHECK(mod_best_defender(veh_at(6, 6), va, 0) != vs);
    // Designated defender wins in a base.
    Vehs[vs].damage_taken = 0;
    int vw = veh_at(6, 6) == vs ? Vehs[vs].next_veh_id_stack : veh_at(6, 6);
    Vehs[vw].state |= VSTATE_DESIGNATE_DEFENDER;
    CHECK_EQ(mod_best_defender(veh_at(6, 6), va, 0), vw);
}

static void test_promote() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int v = place(ua, 1, 5, 5, MORALE_GREEN);
    promote(v);
    CHECK_EQ(Vehs[v].morale, MORALE_DISCIPLINED);
    promote(v);
    CHECK_EQ(Vehs[v].morale, MORALE_HARDENED);
    Vehs[v].morale = MORALE_ELITE;
    promote(v);
    CHECK_EQ(Vehs[v].morale, MORALE_ELITE);
    // Missiles and non-combat units are not promoted.
    int vc = place(BSC_COLONY_POD, 1, 5, 5, 0);
    promote(vc);
    CHECK_EQ(Vehs[vc].morale, 0);
}

static void test_interceptor() {
    setup();
    int fighter = make_unit(unit_slot(2, 1), CHS_NEEDLEJET, WPN_GATLING_LASER, ARM_NO_ARMOR, 1, ABL_AIR_SUPERIORITY);
    int ud = make_unit(unit_slot(2, 2), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    add_base(2, 8, 8, 2);
    int vf = place(fighter, 2, 8, 8);
    place(ud, 2, 6, 6);
    CHECK_EQ(interceptor(2, 1, 6, 6), 1);
    CHECK_EQ(Vehs[vf].x, 6);
    CHECK_EQ(Vehs[vf].y, 6);
    // Out of range (intercept_max_range 2): nothing scrambles.
    CHECK_EQ(interceptor(2, 1, 14, 14), 0);
}

static void test_artifact_relic() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int va = place(ua, 1, 5, 5);
    int vr = place(BSC_ALIEN_ARTIFACT, 2, 6, 6);
    (void)vr;
    BattleResult* r = new_result();
    game_srand(1);
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_ATTACKER_WON);
    CHECK_EQ(r->num_relics, 1);
    int v = veh_at(5, 5);
    bool found = false;
    for (; v >= 0; v = Vehs[v].next_veh_id_stack) {
        if (Vehs[v].unit_id == BSC_ALIEN_ARTIFACT) {
            found = Vehs[v].faction_id == 1;
        }
    }
    CHECK(found);
}

static void test_planet_buster() {
    setup();
    int ub = make_unit(unit_slot(1, 1), CHS_MISSILE, WPN_PLANET_BUSTER, ARM_NO_ARMOR, 1, 0, PLAN_PLANET_BUSTER);
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 1);
    add_base(2, 10, 10, 4);
    add_base(2, 17, 17, 2);
    int vb = place(ub, 1, 9, 9);
    place(ud, 2, 10, 10);
    place(ud, 2, 11, 11);
    place(ud, 2, 15, 15); // outside the blast
    BattleResult* r = new_result();
    CHECK_EQ(engine_attack(vb, 10, 10, r), BATTLE_PLANET_BUSTER);
    CHECK_EQ(base_at(10, 10), -1);
    CHECK_EQ(*BaseCount, 1);
    CHECK_EQ(veh_at(11, 11), -1);
    CHECK(veh_at(15, 15) >= 0);
    CHECK_EQ(*VehCount, 1); // only the far unit; the missile is spent
    CHECK_EQ(r->veh_id_atk, -1);
}

static void test_psi_battle() {
    setup();
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_SILKSTEEL_ARMOR, 1);
    int vw = place(BSC_MIND_WORMS, 0, 5, 5);
    int vd = place(ud, 2, 6, 6, MORALE_DISCIPLINED);
    int off = 0, dfn = 0;
    mod_battle_compute(vw, vd, &off, &dfn, 0);
    // psi: ratios atk 3 (x8), def 2 (x8), native morale at turn 100 is 2.
    CHECK_EQ(off, 3 * 8 * (2 + 6) * 4);
    CHECK_EQ(dfn, 2 * 8 * (2 + 6) * 4);
    game_srand(9);
    BattleResult* r = new_result();
    int out = engine_attack(vw, 6, 6, r);
    CHECK(out == BATTLE_ATTACKER_WON || out == BATTLE_DEFENDER_WON);
    if (out == BATTLE_ATTACKER_WON) {
        CHECK(r->credits == 0); // planetpearls only when natives are killed
    }
    // Faction attacker killing a native earns planetpearls (original: 10 * (lifecycle + 1)).
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_SINGULARITY_LASER, ARM_NO_ARMOR, 4);
    int va = place(ua, 1, 5, 5, 6);
    place(BSC_MIND_WORMS, 0, 6, 6);
    int credits = Factions[1].energy_credits;
    game_srand(2);
    out = engine_attack(va, 6, 6, r);
    if (out == BATTLE_ATTACKER_WON) {
        CHECK_EQ(r->credits, 10 * (2 + 1));
        CHECK_EQ(Factions[1].energy_credits, credits + 30);
    }
}

static void test_invalid() {
    setup();
    BattleResult* r = new_result();
    CHECK_EQ(engine_attack(-1, 5, 5, r), BATTLE_INVALID);
    CHECK_EQ(engine_attack(0, 5, 5, r), BATTLE_INVALID);
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int va = place(ua, 1, 5, 5);
    CHECK_EQ(engine_attack(va, 7, 7, r), BATTLE_NO_TARGET);
    int vb = place(ua, 1, 6, 6);
    (void)vb;
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_NO_TARGET); // own unit
    Rules->move_rate_roads = 0;
    CHECK_EQ(engine_attack(va, 6, 6, r), BATTLE_NO_RULES);
    CHECK(strcmp(battle_outcome_name(BATTLE_PLANET_BUSTER), "planet_buster") == 0);
}

/* Odds preview changes nothing. */
static void test_odds_pure() {
    setup();
    int ua = make_unit(unit_slot(1, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int ud = make_unit(unit_slot(2, 1), CHS_INFANTRY, WPN_LASER, ARM_NO_ARMOR, 1);
    int va = place(ua, 1, 5, 5);
    place(ud, 2, 6, 6);
    std::vector<uint8_t> before((uint8_t*)Vehs, (uint8_t*)(Vehs + *VehCount));
    uint32_t rng = game_rand_state();
    int def = -1;
    CHECK(engine_attack_odds(va, 6, 6, &def) > 0);
    CHECK(def >= 0);
    std::vector<uint8_t> after((uint8_t*)Vehs, (uint8_t*)(Vehs + *VehCount));
    CHECK(before == after);
    CHECK_EQ(game_rand_state(), rng);
}

int main(int argc, char** argv) {
    if (argc < 2 || strcmp(argv[1], "unit") != 0) {
        fprintf(stderr, "usage: wp4_test unit\n");
        return 2;
    }
    test_psi_factor();
    test_terrain_defense();
    test_native_morale();
    test_basic_strengths();
    test_compute_matches_record();
    test_duels();
    test_sensor();
    test_difficulty();
    test_artillery_stage();
    test_retreat();
    test_bombardment();
    test_collateral_and_base();
    test_capture();
    test_best_defender();
    test_promote();
    test_interceptor();
    test_artifact_relic();
    test_planet_buster();
    test_psi_battle();
    test_invalid();
    test_odds_pure();
    state.reset();
    if (failures) {
        fprintf(stderr, "%d of %d checks failed\n", failures, checks);
        return 1;
    }
    printf("%d checks passed, %d oracle cases, prebattle oracles %s\n", checks, oracle_cases,
        wp4o::have_prebattle() ? "on" : "off");
    return 0;
}
