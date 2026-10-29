/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 research, social engineering and unit design checks
 * (driven by test_engine_wp5.py).
 *
 *   wp5_test unit           synthetic-rules unit checks; prints "<n> unit checks passed"
 *   wp5_test protocost IN   prototype cost/derivation queries (see test_engine_wp5.py)
 *   wp5_test techrate IN    original research cost queries
 */
#include "../engine.h"
#include "../tech.h"
#include "../design.h"
#include "../wp5_deps.h"

#include <memory>
#include <string>

using namespace alpha::engine;

static int checks = 0;
static int failures = 0;

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

#define CHECK_STR(a, b) do { \
    checks++; \
    if (strcmp((a), (b)) != 0) { \
        failures++; \
        fprintf(stderr, "%s:%d: CHECK_STR failed: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); \
    } \
} while (0)

static std::unique_ptr<EngineState> g_state;

static char* S(const char* text) {
    return const_cast<char*>(text);
}

/* ------------------------------------------------------------------------
 * Synthetic rules. Technologies 0..6 are roots, tech i >= 7 needs i-7 (and
 * i-6 for even i); tech 80 is disabled. Categories rotate by id % 4.
 * --------------------------------------------------------------------- */
static void setup_rules() {
    g_state.reset(new EngineState());
    engine_reset(*g_state);
    engine_bind(g_state.get());
    config_vanilla(conf);
    *ExpansionEnabled = 1;
    *MapAreaX = 16;
    *MapAreaY = 8;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = (ALT_SHORE_LINE + 1) << 5; // land
        (*MapTiles)[i].region = 1;
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        CTech& t = Tech[i];
        t.name = S("Tech");
        t.flags = 0;
        t.preq_tech1 = (i < 7) ? TECH_None : i - 7;
        t.preq_tech2 = (i < 7 || i % 2) ? TECH_None : i - 6;
        t.AI_growth = (i % 4 == 0) ? 3 : 1;
        t.AI_tech = (i % 4 == 1) ? 3 : 1;
        t.AI_wealth = (i % 4 == 2) ? 3 : 1;
        t.AI_power = (i % 4 == 3) ? 3 : 1;
    }
    Tech[80].preq_tech1 = TECH_Disable;
    Tech[80].preq_tech2 = TECH_Disable;
    Rules->tech_discovery_rate = 100;
    Rules->extra_cost_prototype_land = 50;
    Rules->extra_cost_prototype_sea = 50;
    Rules->extra_cost_prototype_air = 50;
    // Chassis: infantry, speeder, hovertank, foil, cruiser, needlejet, copter, gravship, missile
    const int speeds[] = {1, 2, 3, 4, 6, 8, 8, 8, 12};
    const int triads[] = {0, 0, 0, 1, 1, 2, 2, 2, 2};
    for (int i = 0; i < MaxChassisNum; i++) {
        CChassis& c = Chassis[i];
        c.offsv1_name = S(i == 0 ? "Squad" : "Rover");
        c.offsv2_name = S(i == 0 ? "Infantry" : "Speeder");
        c.offsv_name_lrg = S("Dreadnought");
        c.defsv1_name = S("Defender");
        c.defsv2_name = S("Sentinel");
        c.defsv_name_lrg = S("Bulwark");
        c.speed = (uint8_t)speeds[i];
        c.triad = (uint8_t)triads[i];
        c.range = (uint8_t)(triads[i] == 2 && i != 8 ? 2 : 0);
        c.cargo = (uint8_t)(i == 3 ? 4 : i == 4 ? 8 : 1);
        c.cost = (uint8_t)speeds[i];
        c.missile = (uint8_t)(i == 8);
        c.preq_tech = (int16_t)(i == 0 ? TECH_None : i);
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        CWeapon& w = Weapon[i];
        w.name = S("Weapon");
        w.name_short = S(i == 0 ? "Hand" : i == 1 ? "Laser" : "Gun");
        w.offense_value = (int8_t)(i + 1);
        w.mode = (uint8_t)(i % 2 ? WMODE_ENERGY : WMODE_PROJECTILE);
        w.cost = (uint8_t)(i + 1);
        w.preq_tech = (int16_t)(i == 0 ? TECH_None : 10 + i);
    }
    const int noncombat[][2] = {{WPN_COLONY_MODULE, WMODE_COLONY}, {WPN_TERRAFORMING_UNIT, WMODE_TERRAFORM},
        {WPN_TROOP_TRANSPORT, WMODE_TRANSPORT}, {WPN_SUPPLY_TRANSPORT, WMODE_SUPPLY},
        {WPN_PROBE_TEAM, WMODE_PROBE}};
    for (auto& n : noncombat) {
        Weapon[n[0]].offense_value = 0;
        Weapon[n[0]].mode = (uint8_t)n[1];
        Weapon[n[0]].cost = 1;
        Weapon[n[0]].preq_tech = TECH_None;
    }
    Weapon[WPN_TERRAFORMING_UNIT].name_short = S("Formers");
    Weapon[WPN_TROOP_TRANSPORT].name_short = S("Transport");
    for (int i = 0; i < MaxArmorNum; i++) {
        CArmor& a = Armor[i];
        a.name = S("Armor");
        a.name_short = S(i == 0 ? "None" : "Plate");
        a.defense_value = (int8_t)(i + 1);
        a.cost = (uint8_t)(i + 1);
        a.preq_tech = (int16_t)(i == 0 ? TECH_None : 20 + i);
    }
    for (int i = 0; i < MaxReactorNum; i++) {
        Reactor[i].name = S("Reactor");
        Reactor[i].preq_tech = (int16_t)(i == 0 ? TECH_None : 40 + i);
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        Ability[i].name = S("Ability");
        Ability[i].abbreviation = S("");
        Ability[i].cost = 1;
        Ability[i].preq_tech = TECH_None;
    }
    for (int sf = 0; sf < MaxSocialCatNum; sf++) {
        SocialField[sf].field_name = S("Field");
        for (int sm = 0; sm < MaxSocialModelNum; sm++) {
            SocialField[sf].soc_name[sm] = S("Model");
            SocialField[sf].soc_preq_tech[sm] = (int16_t)(sm == 0 ? TECH_None : 30 + 4 * sf + sm);
            memset(&SocialField[sf].soc_effect[sm], 0, sizeof(CSocialEffect));
        }
    }
    for (int i = 0; i <= SP_ID_Last; i++) {
        Facility[i].preq_tech = TECH_Disable;
    }
    for (int i = 0; i < MaxTerrainNum; i++) {
        Terraform[i].preq_tech = TECH_Disable;
    }
    for (int i = 0; i < 64; i++) {
        SecretProjects[i] = SP_Unbuilt;
    }
    for (int i = 0; i < MaxProtoNum; i++) {
        memset(&Units[i], 0, sizeof(UNIT));
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        set_alive(i, true);
        Factions[i].diff_level = 3;
        Factions[i].tech_research_id = -1;
        Factions[i].tech_cost = -1;
        Factions[i].target_x = -1;
        MFactions[i].rule_techcost = 100;
        MFactions[i].soc_opposition_category = -1;
        MFactions[i].soc_opposition_model = -1;
        strcpy(MFactions[i].noun_faction, "Faction");
    }
    *DiffLevel = 3;
    *CurrentTurn = 10;
    MapWin->cOwner = 1;
    set_human(1, true);
    headless_sink()->clear();
    headless_sink()->choices.clear();
    headless_sink()->default_choice = 0;
    set_tech_chooser(nullptr, nullptr);
    set_social_chooser(nullptr, nullptr);
}

static void grant(int faction_id, int tech_id) {
    TechOwners[tech_id] |= (1 << faction_id);
}

/* ------------------------------------------------------------------------ */

static void test_tech_avail() {
    setup_rules();
    CHECK(tech_avail(0, 1));
    CHECK(!tech_avail(7, 1));
    grant(1, 0);
    CHECK(!tech_avail(0, 1)); // already known
    CHECK(tech_avail(7, 1));  // odd: single prerequisite
    CHECK(!tech_avail(8, 1)); // needs 1 and 2
    grant(1, 1);
    CHECK(!tech_avail(8, 1));
    grant(1, 2);
    CHECK(tech_avail(8, 1));
    CHECK(!tech_avail(80, 1)); // disabled
    CHECK(!tech_avail(-1, 1));
    CHECK(!tech_avail(89, 1));
    CHECK(!tech_avail(0, 9));
    // Base game excludes the Alien Crossfire techs
    Tech[TECH_PrPsych].preq_tech1 = TECH_None;
    Tech[TECH_PrPsych].preq_tech2 = TECH_None;
    CHECK(tech_avail(TECH_PrPsych, 1));
    *ExpansionEnabled = 0;
    CHECK(!tech_avail(TECH_PrPsych, 1));
    *ExpansionEnabled = 1;

    CHECK_EQ(tech_level(0, 0), 1);
    CHECK_EQ(tech_level(7, 0), 2);
    CHECK_EQ(tech_level(14, 0), 3);
    CHECK_EQ(tech_level(-1, 0), 0);
    CHECK_EQ(tech_category(0), TCAT_GROWTH);
    CHECK_EQ(tech_category(1), TCAT_TECH);
    CHECK_EQ(tech_category(2), TCAT_WEALTH);
    CHECK_EQ(tech_category(3), TCAT_POWER);
    Tech[5].AI_growth = Tech[5].AI_tech = Tech[5].AI_wealth = Tech[5].AI_power = 2;
    CHECK_EQ(tech_category(5), TCAT_GROWTH); // tie prefers growth
    Tech[5].AI_growth = 1;
    CHECK_EQ(tech_category(5), TCAT_TECH);
    CHECK(tech_is_preq(0, 14, 2));
    CHECK(!tech_is_preq(0, 14, 1));
    CHECK(tech_is_preq(14, 14, 0));
    CHECK(!tech_is_preq(3, 14, 5));

    char buf[StrBufLen] = "";
    say_tech(buf, -1, 0);
    CHECK_STR(buf, "NONE");
    buf[0] = 0;
    say_tech(buf, 9999, 0);
    CHECK_STR(buf, "World Map");
    buf[0] = 0;
    say_tech(buf, 7, 1);
    CHECK_STR(buf, "Tech (C2)"); // 7 % 4 == 3: power
    CHECK_STR(tech_str(-1), "-");
}

static void test_tech_effects() {
    setup_rules();
    Tech[0].flags = TFLAG_INC_NUTRIENT_FUNGUS | TFLAG_INC_COMMERCE;
    Tech[1].flags = TFLAG_INC_MINERALS_FUNGUS | TFLAG_INC_ENERGY_FUNGUS;
    Tech[2].flags = TFLAG_INC_ENERGY_FUNGUS | TFLAG_INC_COMMERCE;
    grant(2, 0);
    grant(2, 1);
    grant(2, 2);
    MFactions[2].rule_commerce = 1;
    MFactions[2].faction_bonus_count = 2;
    MFactions[2].faction_bonus_id[0] = RULE_FUNGMINERALS;
    MFactions[2].faction_bonus_val1[0] = -3;
    MFactions[2].faction_bonus_id[1] = RULE_FUNGNUTRIENT;
    MFactions[2].faction_bonus_val1[1] = 2;
    Factions[2].SE_economy_pending = 4;
    tech_effects(2);
    CHECK_EQ(Factions[2].tech_commerce_bonus, 1 + 2 + 2);
    CHECK_EQ(Factions[2].tech_fungus_nutrient, 3);
    CHECK_EQ(Factions[2].tech_fungus_mineral, 0); // clamped
    CHECK_EQ(Factions[2].tech_fungus_energy, 2);
    Factions[2].SE_economy_pending = 5;
    tech_effects(2);
    CHECK_EQ(Factions[2].tech_commerce_bonus, 1 + 2 + 3);
}

/* Independent model of the directed human pool (RESEARCH-POOL rules). */
static int pool_model(int faction_id, int* ids) {
    int n = 0;
    int ranking = Factions[faction_id].tech_ranking;
    for (int group = 0; group < 4; group++) {
        for (int id = 0; id < MaxTechnologyNum; id++) {
            if (!tech_avail(id, faction_id)) continue;
            long long v = (long long)id - ranking + faction_id;
            bool withheld = n > 0 && v % 3 == 0 && !(id < 7 && ranking == 0);
            if (withheld) continue;
            if (tech_category(id) != group) continue;
            ids[n++] = id;
        }
    }
    return n;
}

static int g_chooser_calls = 0;
static int g_chooser_answer = -1;
static int test_chooser(const TechChoice& c, void* ctx) {
    g_chooser_calls++;
    *(int*)ctx = c.count;
    return g_chooser_answer;
}

static void test_research_pool_and_pick() {
    setup_rules();
    TechChoice c;
    int ids[MaxTechnologyNum];
    // ranking 0: slots 0..6 exempt from the rotation
    int n = research_pool(1, -1, &c);
    CHECK_EQ(n, 7);
    CHECK_EQ(n, pool_model(1, ids));
    CHECK_EQ(c.ids[0], 0);
    CHECK_EQ(c.ids[1], 4);
    CHECK_EQ(c.categories[1], TCAT_GROWTH);
    CHECK_EQ(c.ids[2], 1);
    for (int ranking = 0; ranking < 12; ranking++) {
        for (int fac = 1; fac < 4; fac++) {
            setup_rules();
            for (int t = 0; t < 7 + ranking; t++) grant(fac, t);
            Factions[fac].tech_ranking = ranking;
            int m = pool_model(fac, ids);
            int k = research_pool(fac, -1, &c);
            CHECK_EQ(k, m);
            bool same = true;
            for (int i = 0; i < k && i < m; i++) same = same && c.ids[i] == ids[i];
            CHECK(same);
            CHECK(k > 0);
        }
    }
    // Trade list
    setup_rules();
    grant(2, 0); grant(2, 1); grant(2, 7); grant(1, 0);
    n = research_pool(1, 2, &c);
    CHECK_EQ(n, 2); // 1 and 7 (0 known, 7 needs 0 which 1 has)
    CHECK_EQ(c.ids[0], 1);
    CHECK_EQ(c.ids[1], 7);
    int tr[4];
    CHECK_EQ(tech_tradeable(1, 2, tr, 4), 2);
    CHECK_EQ(tr[1], 7);

    // Human pick through the chooser hook
    setup_rules();
    int count = 0;
    set_tech_chooser(test_chooser, &count);
    g_chooser_answer = 4;
    CHECK_EQ(tech_pick(1, 0, -1, nullptr), 4);
    CHECK_EQ(count, 7);
    g_chooser_answer = 50; // not offered: cancel
    CHECK_EQ(tech_pick(1, 0, -1, nullptr), -1);
    g_chooser_answer = 1;
    CHECK_EQ(mod_tech_selection(1), 1);
    CHECK_EQ(Factions[1].tech_research_id, 1);
    set_tech_chooser(nullptr, nullptr);
    // Default: popup through the event sink, answer = list index
    headless_sink()->choices["TECHPICK"] = 2;
    CHECK_EQ(tech_pick(1, 0, -1, nullptr), 1);
    headless_sink()->choices["TECHPICK"] = -1;
    CHECK_EQ(tech_pick(1, 0, -1, nullptr), -1);
    // Blind research and turn 0 use the automatic choice, always researchable
    *GameRules |= RULES_BLIND_RESEARCH;
    int t = tech_pick(1, 0, -1, nullptr);
    CHECK(tech_avail(t, 1));
    *GameRules &= ~RULES_BLIND_RESEARCH;
    *CurrentTurn = 0;
    t = tech_pick(1, 0, -1, nullptr);
    CHECK(tech_avail(t, 1));
    *CurrentTurn = 10;
    // Computer player
    t = tech_pick(3, 0, -1, nullptr);
    CHECK(tech_avail(t, 3));
    // Nothing to research
    for (int i = 0; i < MaxTechnologyNum; i++) grant(4, i);
    CHECK_EQ(tech_pick(4, 0, -1, nullptr), TECH_TranT); // repeatable
    TechChoice none;
    Tech[TECH_TranT].preq_tech1 = TECH_Disable;
    CHECK_EQ(research_pool(4, -1, &none), 0);
    CHECK_EQ(tech_pick(4, 0, -1, nullptr), -1);
    // Steal from a faction with nothing new: -1
    CHECK_EQ(tech_pick(3, 0, 5, "STEALTECH"), -1);
    grant(5, 2);
    CHECK_EQ(tech_pick(3, 0, 5, "STEALTECH"), 2);
}

static void test_research_change() {
    setup_rules();
    Factions[1].tech_research_id = -1;
    Factions[1].tech_accumulated = 9;
    Factions[1].tech_cost = 77;
    CHECK(!research_change_needs_confirmation(1));
    CHECK_EQ(research_change(1, 3, false), RESEARCH_SET);
    CHECK_EQ(Factions[1].tech_research_id, 3);
    CHECK_EQ(Factions[1].tech_accumulated, 9);
    CHECK_EQ(research_change(1, 3, false), RESEARCH_SAME);
    CHECK(research_change_needs_confirmation(1));
    CHECK_EQ(research_change(1, 4, false), RESEARCH_NEEDS_CONFIRMATION);
    CHECK_EQ(Factions[1].tech_research_id, 3);
    CHECK_EQ(research_change(1, 4, true), RESEARCH_CHANGED);
    CHECK_EQ(Factions[1].tech_research_id, 4);
    CHECK_EQ(Factions[1].tech_accumulated, 4);
    CHECK_EQ(Factions[1].tech_cost, 77); // cached cost kept
    Factions[1].tech_accumulated = -7;
    CHECK_EQ(research_change(1, 5, true), RESEARCH_CHANGED);
    CHECK_EQ(Factions[1].tech_accumulated, -3); // truncates toward zero
    CHECK_EQ(research_change(1, 7, true), RESEARCH_INVALID);
    Factions[1].tech_accumulated = 0;
    CHECK_EQ(research_change(1, 6, false), RESEARCH_CHANGED); // no labs: no confirmation
    CHECK_EQ(Factions[1].tech_accumulated, 0);
}

static void test_tech_rate() {
    setup_rules();
    *MapAreaSqRoot = 56;
    Factions[1].tech_cost = 123;
    CHECK_EQ(mod_tech_rate(1), 123);
    Factions[1].tech_cost = -1;
    Rules->tech_discovery_rate = 0;
    CHECK_EQ(tech_rate(1), 999999999);
    Rules->tech_discovery_rate = 100;
    // Human, Librarian, nothing known yet: rating 2 -> half 1, factor 12+1=13 clamp(11..13)
    CHECK(tech_rate(1) > 0);
    conf.revised_tech_cost = 1;
    Factions[1].tech_cost = 0;
    CHECK_EQ(mod_tech_rate(1), 9999);
    conf.revised_tech_cost = 0;
}

static void test_tech_achieved() {
    setup_rules();
    Factions[1].tech_research_id = 0;
    Factions[1].unk_27 = 1;
    tech_achieved(1, 0, 0, 0);
    CHECK(has_tech(0, 1));
    CHECK_EQ(Factions[1].tech_achieved, 1);
    CHECK_EQ(Factions[1].tech_ranking, 2);
    CHECK_EQ(Factions[1].unk_27, 0);
    CHECK_EQ(Factions[1].tech_research_id, -1);
    CHECK_EQ(Factions[1].tech_trade_source[0], 0);
    // Trade source and turn 0 (no ranking)
    tech_achieved(1, 1, 3, 0);
    CHECK_EQ(Factions[1].tech_trade_source[1], 3);
    *CurrentTurn = 0;
    tech_achieved(1, 2, 0, 0);
    CHECK_EQ(Factions[1].tech_ranking, 4);
    CHECK_EQ(Factions[1].tech_achieved, 3);
    *CurrentTurn = 10;
    *SkipTechScreenA = 1;
    tech_achieved(1, 3, 0, 0);
    CHECK_EQ(Factions[1].tech_ranking, 4);
    *SkipTechScreenA = 0;
    // Transcendent thought counter
    Tech[TECH_TranT].preq_tech1 = TECH_None;
    Tech[TECH_TranT].preq_tech2 = TECH_None;
    tech_achieved(2, TECH_TranT, 0, 0);
    CHECK_EQ(Factions[2].tech_count_transcendent, 1);
    // Map reveal flag
    Tech[4].flags = TFLAG_REVEALS_MAP;
    (*MapTiles)[5].items = 0x55;
    tech_achieved(2, 4, 0, 0);
    CHECK(Factions[2].player_flags & PFLAG_MAP_REVEALED);
    CHECK((*MapTiles)[5].visibility & (1 << 2));
    CHECK_EQ((*MapTiles)[5].visible_items[1], 0x55);
    // Secrets: first discoverer gets the bonus flag, the second does not
    Tech[5].flags = TFLAG_SECRETS;
    tech_achieved(3, 5, 0, 0);
    CHECK(Factions[3].player_flags & PFLAG_FIRST_SECRETS);
    tech_achieved(4, 5, 0, 0);
    CHECK(!(Factions[4].player_flags & PFLAG_FIRST_SECRETS));
    // Popup text for the local player
    headless_sink()->clear();
    tech_achieved(1, 6, 2, 0);
    bool found = false;
    for (auto& line : headless_sink()->log) {
        if (line.find("WEACQUIRE") != std::string::npos) found = true;
    }
    CHECK(found);
}

static void test_tech_achieved_propagation() {
    // TECHSHARE: faction 3 gets a tech known by 2 other factions
    setup_rules();
    MFactions[3].rule_sharetech = 2;
    tech_achieved(1, 0, 0, 0);
    CHECK(!has_tech(0, 3));
    tech_achieved(2, 0, 0, 0);
    CHECK(has_tech(0, 3));
    CHECK_EQ(Factions[3].tech_trade_source[0], 0);
    // TECHSHARE with RFLAG_TECHSHARE requires infiltration or pact
    setup_rules();
    MFactions[3].rule_sharetech = 2;
    MFactions[3].rule_flags = RFLAG_TECHSHARE;
    tech_achieved(1, 0, 0, 0);
    tech_achieved(2, 0, 0, 0);
    CHECK(!has_tech(0, 3));
    Factions[3].diplo_status[4] |= DIPLO_PACT;
    Factions[3].diplo_status[1] |= DIPLO_HAVE_INFILTRATOR;
    tech_achieved(4, 0, 0, 0);
    CHECK(has_tech(0, 3));
    // Planetary Datalinks: owner gets a tech known by 3 factions
    setup_rules();
    int b = base_init(5, 4, 4);
    CHECK(b >= 0);
    SecretProjects[FAC_PLANETARY_DATALINKS - SP_ID_First] = b;
    tech_achieved(1, 2, 0, 0);
    tech_achieved(2, 2, 0, 0);
    CHECK(!has_tech(2, 5));
    tech_achieved(3, 2, 0, 0);
    CHECK(has_tech(2, 5));
    // Free facility bonus is granted to every base of the faction
    setup_rules();
    int b1 = base_init(2, 2, 2);
    int b2 = base_init(2, 8, 2);
    CHECK(b1 >= 0 && b2 >= 0);
    Facility[FAC_RECYCLING_TANKS].preq_tech = 3;
    MFactions[2].faction_bonus_count = 1;
    MFactions[2].faction_bonus_id[0] = RULE_FREEFAC;
    MFactions[2].faction_bonus_val1[0] = FAC_RECYCLING_TANKS;
    tech_achieved(2, 3, 0, 0);
    CHECK(has_fac_built(FAC_RECYCLING_TANKS, b1));
    CHECK(has_fac_built(FAC_RECYCLING_TANKS, b2));
    // World map trade
    setup_rules();
    (*MapTiles)[3].visibility = 1 << 4;
    (*MapTiles)[3].items = 0x0F;
    (*MapTiles)[3].visible_items[3] = 0x03;
    tech_achieved(2, 9999, 4, 0);
    CHECK((*MapTiles)[3].visibility & (1 << 2));
    CHECK_EQ((*MapTiles)[3].visible_items[1], 0x03);
    CHECK(Factions[2].traded_maps & (1 << 4));
}

static void test_tech_advance_steal() {
    setup_rules();
    // AI research completes its current target
    Factions[3].tech_research_id = 2;
    CHECK_EQ(tech_advance(3), 2);
    CHECK(has_tech(2, 3));
    // Human local player: next target chosen immediately
    int count = 0;
    set_tech_chooser(test_chooser, &count);
    g_chooser_answer = 0;
    Factions[1].tech_research_id = 4;
    CHECK_EQ(tech_advance(1), 4);
    CHECK(has_tech(4, 1));
    CHECK_EQ(Factions[1].tech_research_id, 0);
    set_tech_chooser(nullptr, nullptr);
    // Steal: AI takes a tech of the target
    grant(5, 6);
    CHECK_EQ(steal_tech(3, 5, 1), 0);
    CHECK(has_tech(6, 3));
    CHECK_EQ(Factions[3].tech_trade_source[6], 5);
    // Nothing to steal: the world map is taken
    CHECK_EQ(steal_tech(3, 6, 1), 1);
    CHECK(Factions[3].traded_maps & (1 << 6));
    CHECK_EQ(steal_tech(0, 5, 1), 0);
}

static void test_social() {
    setup_rules();
    SocialField[0].soc_effect[1].values[SE_ECONOMY] = 2;
    SocialField[0].soc_effect[1].values[SE_POLICE] = -2;
    SocialField[1].soc_effect[2].values[SE_GROWTH] = -1;
    SocialField[1].soc_effect[2].values[SE_RESEARCH] = 1;
    CSocialCategory cat = {};
    cat.models[0] = 1;
    cat.models[1] = 2;
    CSocialEffect eff;
    social_calc(&cat, &eff, 1, 0, 0);
    CHECK_EQ(eff.economy, 2);
    CHECK_EQ(eff.police, -2);
    CHECK_EQ(eff.growth, -1);
    CHECK_EQ(eff.research, 1);
    // IMPUNITY negates, PENALTY doubles the negative effects of one model
    MFactions[1].faction_bonus_count = 2;
    MFactions[1].faction_bonus_id[0] = RULE_IMPUNITY;
    MFactions[1].faction_bonus_val1[0] = 0;
    MFactions[1].faction_bonus_val2[0] = 1;
    MFactions[1].faction_bonus_id[1] = RULE_PENALTY;
    MFactions[1].faction_bonus_val1[1] = 1;
    MFactions[1].faction_bonus_val2[1] = 2;
    social_calc(&cat, &eff, 1, 0, 0);
    CHECK_EQ(eff.police, 0);
    CHECK_EQ(eff.growth, -2);
    // SOCIAL base values, ROBUST halves, IMMUNITY cancels
    setup_rules();
    SocialField[0].soc_effect[1].values[SE_ECONOMY] = -3;
    SocialField[0].soc_effect[1].values[SE_PROBE] = -2;
    MFactions[2].faction_bonus_count = 3;
    MFactions[2].faction_bonus_id[0] = RULE_SOCIAL;
    MFactions[2].faction_bonus_val1[0] = SE_RESEARCH;
    MFactions[2].faction_bonus_val2[0] = 2;
    MFactions[2].faction_bonus_id[1] = RULE_ROBUST;
    MFactions[2].faction_bonus_val1[1] = SE_ECONOMY;
    MFactions[2].faction_bonus_id[2] = RULE_IMMUNITY;
    MFactions[2].faction_bonus_val1[2] = SE_PROBE;
    compute_faction_modifiers(2);
    CHECK_EQ(Factions[2].SE_research_base, 2);
    cat = {};
    cat.models[0] = 1;
    social_calc(&cat, &eff, 2, 0, 0);
    CHECK_EQ(eff.research, 2);
    CHECK_EQ(eff.economy, -1);
    CHECK_EQ(eff.probe, 0);
    social_calc(&cat, &eff, 2, 0, 1); // quick calc: no base values or immunity
    CHECK_EQ(eff.research, 0);
    CHECK_EQ(eff.economy, -3);
    CHECK_EQ(eff.probe, -2);
    // Availability: technology and opposition
    CHECK(society_avail(0, 0, 1));
    CHECK(!society_avail(0, 1, 1));
    grant(1, 31);
    CHECK(society_avail(0, 1, 1));
    MFactions[1].soc_opposition_category = 0;
    MFactions[1].soc_opposition_model = 1;
    CHECK(!society_avail(0, 1, 1));
    CHECK(!society_avail(4, 0, 1));
}

static bool g_social_called = false;
static bool social_chooser(int, CSocialCategory* choices, void*) {
    g_social_called = true;
    choices->models[1] = 2;
    return true;
}

static void test_social_change() {
    setup_rules();
    SocialField[0].soc_effect[1].values[SE_ECONOMY] = 2;
    Factions[1].energy_credits = 1000;
    CHECK_EQ(social_change(1, 0, 1), -1); // not available
    grant(1, 31);
    CSocialCategory next = {};
    next.models[0] = 1;
    int expected = social_upheaval(1, &next);
    int paid = social_change(1, 0, 1);
    CHECK_EQ(paid, expected);
    CHECK_EQ(Factions[1].SE_Politics_pending, 1);
    CHECK_EQ(Factions[1].SE_Politics, 0);
    CHECK_EQ(Factions[1].SE_economy_pending, 2);
    CHECK_EQ(Factions[1].energy_credits, 1000 - expected);
    CHECK_EQ(Factions[1].SE_upheaval_cost_paid, expected);
    // Back to the start-of-turn choice: everything paid is refunded
    CHECK_EQ(social_change(1, 0, 0), -expected);
    CHECK_EQ(Factions[1].energy_credits, 1000);
    CHECK_EQ(Factions[1].SE_economy_pending, 0);
    // Screen with a chooser
    grant(1, 30 + 4 + 2);
    set_social_chooser(social_chooser, nullptr);
    social_select(1);
    CHECK(g_social_called);
    CHECK_EQ(Factions[1].SE_Economics_pending, 2);
    set_social_chooser(nullptr, nullptr);
    // Without a chooser: popup, choices kept
    headless_sink()->clear();
    social_select(1);
    CHECK_EQ(Factions[1].SE_Economics_pending, 2);
}

/* ---- Unit design ---- */

static void test_make_and_propose() {
    setup_rules();
    // Basic unit: no prototype shortcut
    make_proto(0, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    CHECK_EQ(Units[0].unit_flags, UNIT_ACTIVE);
    CHECK_EQ(Units[0].cost, proto_cost(CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 0, REC_FISSION));
    CHECK_EQ(Units[0].plan, PLAN_OFFENSE);
    CHECK_EQ(Units[0].group_id, 10);
    // Faction design with known components: no prototype needed
    int u = propose_proto(1, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, ABL_TRAINED, REC_FISSION,
        PLAN_AUTO_CALCULATE, nullptr);
    CHECK_EQ(u, 64);
    CHECK(Units[u].unit_flags & UNIT_PROTOTYPED);
    CHECK(Units[u].unit_flags & UNIT_UNK_100);
    CHECK(Units[u].unit_flags & UNIT_UNK_80);
    // Same components: the existing design is returned
    CHECK_EQ(propose_proto(1, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, ABL_TRAINED, REC_FISSION,
        PLAN_AUTO_CALCULATE, nullptr), 64);
    CHECK_EQ(propose_proto(1, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 0, REC_FISSION,
        PLAN_AUTO_CALCULATE, nullptr), 0); // basic unit 0
    // New weapon: a real prototype, named, preq = most advanced component
    grant(1, 0); grant(1, 5); grant(1, 12); // weapon 2 needs 12 (needs 5, 6?) -> set ranks
    int v = propose_proto(1, CHS_INFANTRY, 2, ARM_NO_ARMOR, 0, REC_FISSION, PLAN_AUTO_CALCULATE, nullptr);
    CHECK_EQ(v, 65);
    CHECK(!(Units[v].unit_flags & UNIT_PROTOTYPED));
    CHECK_EQ(Units[v].preq_tech, 12);
    CHECK_STR(Units[v].name, "Gun Infantry");
    CHECK_EQ(Units[v].plan, PLAN_OFFENSE);
    char nm[] = "Custom";
    int w = propose_proto(1, CHS_INFANTRY, 3, 2, 0, REC_FISSION, PLAN_PROTOTYPE_DONE, nm);
    CHECK_STR(Units[w].name, "Custom");
    CHECK(Units[w].unit_flags & UNIT_PROTOTYPED);
    int x = propose_proto(1, CHS_INFANTRY, 4, 2, 0, REC_FISSION, PLAN_DEFENSE, nullptr);
    CHECK_EQ(Units[x].plan, PLAN_DEFENSE);
    CHECK_EQ(propose_proto(1, 99, 0, 0, 0, 1, -1, nullptr), -1);
    CHECK_EQ(free_proto_slots(1), 64 - 4);
    // Fill every slot, then obsolete ones are reused, otherwise -1
    for (int i = 0; i < 64; i++) {
        propose_proto(2, CHS_INFANTRY, i % 20, 1 + i / 20, 0, REC_FISSION, PLAN_AUTO_CALCULATE, nullptr);
    }
    CHECK_EQ(free_proto_slots(2), 0);
    CHECK_EQ(propose_proto(2, CHS_SPEEDER, 0, 0, 0, REC_FISSION, -1, nullptr), -1);
    retire_proto(130, 2);
    CHECK(!(Units[130].unit_flags & UNIT_ACTIVE)); // no units: deleted
    CHECK_EQ(propose_proto(2, CHS_SPEEDER, 0, 0, 0, REC_FISSION, -1, nullptr), 130);
    int veh = veh_init(131, 2, 3, 3);
    CHECK(veh >= 0);
    retire_proto(131, 2);
    CHECK(Units[131].unit_flags & UNIT_ACTIVE); // units remain: only obsolete
    CHECK(Units[131].obsolete_factions & (1 << 2));
    CHECK_EQ(propose_proto(2, CHS_SPEEDER, 1, 0, 0, REC_FISSION, -1, nullptr), -1);
    retire_proto(132, 2);
    CHECK_EQ(propose_proto(2, CHS_SPEEDER, 2, 0, 0, REC_FISSION, -1, nullptr), 132);
    // veh_avail
    CHECK(veh_avail(0, 1, -1));
    CHECK(!veh_avail(131, 2, -1)); // obsolete
    CHECK(!veh_avail(64, 2, -1)); // other faction
    CHECK(veh_avail(64, 1, -1));
    Units[1] = Units[0];
    Units[1].preq_tech = 70;
    CHECK(!veh_avail(1, 1, -1));
    Units[64].plan = PLAN_COLONY;
    *GameRules |= RULES_SCN_NO_COLONY_PODS;
    CHECK(!veh_avail(64, 1, -1));
    *GameRules &= ~RULES_SCN_NO_COLONY_PODS;
}

static void test_names() {
    setup_rules();
    char name[64];
    name_proto(name, -1, 1, CHS_INFANTRY, WPN_HAND_WEAPONS, ARM_NO_ARMOR, 0, REC_FISSION);
    CHECK_STR(name, "Scout Squad");
    name_proto(name, -1, 1, CHS_SPEEDER, 1, ARM_NO_ARMOR, 0, REC_FISSION);
    CHECK_STR(name, "Recon Speeder");
    name_proto(name, -1, 1, CHS_INFANTRY, WPN_HAND_WEAPONS, 3, 0, REC_FISSION);
    CHECK_STR(name, "Plate Sentinel");
    name_proto(name, -1, 1, CHS_INFANTRY, 3, 2, 0, REC_FISSION);
    CHECK_STR(name, "Plate Gun Squad");
    name_proto(name, -1, 1, CHS_SPEEDER, WPN_TERRAFORMING_UNIT, ARM_NO_ARMOR, 0, REC_FISSION);
    CHECK_STR(name, "Rover Formers");
    name_proto(name, -1, 1, CHS_INFANTRY + 8, 1, ARM_NO_ARMOR, 0, REC_FISSION);
    CHECK_STR(name, "Laser Rover");
    CHECK(strlen(name) < (size_t)MaxProtoNameLen);
    name_proto(name, -1, 1, 99, 0, 0, 0, 1);
    CHECK_STR(name, "");
}

static void test_costs_and_upgrades() {
    setup_rules();
    // prototype factor: free on easy levels and with FREEPROTO
    make_proto(64, CHS_INFANTRY, 2, 1, 0, REC_FISSION);
    Factions[1].diff_level = 3;
    CHECK_EQ(prototype_factor(64), 50);
    Factions[1].diff_level = DIFF_SPECIALIST;
    CHECK_EQ(prototype_factor(64), 0);
    Factions[1].diff_level = 3;
    Units[64].unit_flags = UNIT_ACTIVE;
    int base_cost = mod_base_cost(64);
    int32_t first = 0;
    CHECK_EQ(mod_veh_cost(64, -1, &first), Units[64].cost + (50 * base_cost + 50) / 100);
    CHECK_EQ(first, 1);
    Units[64].unit_flags |= UNIT_PROTOTYPED;
    CHECK_EQ(mod_veh_cost(64, -1, &first), Units[64].cost);
    CHECK_EQ(first, 0);
    // Upgrade cost (original formula)
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    int c = mod_upgrade_cost(1, 64, 0);
    CHECK_EQ(c, max(0, Chassis[0].speed - Chassis[0].speed) + (Armor[1].cost - Armor[0].cost)
        + (Weapon[2].cost - Weapon[0].cost) + Units[64].cost);
    Units[64].unit_flags &= ~UNIT_PROTOTYPED;
    int m = Units[64].cost;
    CHECK_EQ(mod_upgrade_cost(1, 64, 0), (Armor[1].cost - Armor[0].cost) + (Weapon[2].cost - Weapon[0].cost)
        + ((m + 1) / 2 + m) * ((m + 1) / 2 + m + 1));
    Units[64].unit_flags |= UNIT_PROTOTYPED;
    // Full upgrade: vehicles, queues, energy
    Units[0].preq_tech = TECH_None;
    int b = base_init(1, 6, 4);
    CHECK(b >= 0);
    Bases[b].queue_size = 0;
    Bases[b].queue_items[0] = 0;
    int v1 = veh_init(0, 1, 2, 2);
    int v2 = veh_init(0, 1, 3, 3);
    CHECK(v1 >= 0 && v2 >= 0);
    Factions[1].energy_credits = 1000;
    full_upgrade(1, 64, 0);
    CHECK_EQ(Vehs[v1].unit_id, 64);
    CHECK_EQ(Vehs[v2].unit_id, 64);
    CHECK_EQ(Bases[b].queue_items[0], 64);
    CHECK_EQ(Factions[1].energy_credits, 1000 - 2 * 10 * c);
    CHECK(Units[0].unit_flags & UNIT_ACTIVE); // basic units stay
    // AI upgrade threshold
    setup_rules();
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    make_proto(3 * 64, CHS_INFANTRY, 1, 0, 0, REC_FISSION);
    Units[3 * 64].unit_flags |= UNIT_PROTOTYPED;
    int va = veh_init(0, 3, 2, 2);
    CHECK(va >= 0);
    Factions[3].energy_credits = 0;
    CHECK_EQ(mod_upgrade_prototype(3, 3 * 64, 0, 1), 0);
    Factions[3].energy_credits = 10000;
    CHECK_EQ(mod_upgrade_prototype(3, 3 * 64, 0, 1), 1);
    CHECK_EQ(Vehs[va].unit_id, 3 * 64);
    CHECK_EQ(mod_upgrade_prototype(3, 0, 3 * 64, 1), 0); // basic target rejected
    // Human local player confirms through the popup
    setup_rules();
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    make_proto(64, CHS_INFANTRY, 1, 0, 0, REC_FISSION);
    Units[64].unit_flags |= UNIT_PROTOTYPED;
    int vh = veh_init(0, 1, 2, 2);
    Factions[1].energy_credits = 10000;
    headless_sink()->choices["UPGRADEPROTO"] = 0;
    CHECK_EQ(mod_upgrade_prototype(1, 64, 0, 0), 0);
    CHECK_EQ(Vehs[vh].unit_id, 0);
    headless_sink()->choices["UPGRADEPROTO"] = 1;
    CHECK_EQ(mod_upgrade_prototype(1, 64, 0, 0), 1);
    CHECK_EQ(Vehs[vh].unit_id, 64);
    // Partial upgrade only at bases with moves left
    setup_rules();
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    make_proto(64, CHS_INFANTRY, 1, 0, 0, REC_FISSION);
    Units[64].unit_flags |= UNIT_PROTOTYPED;
    int pb = base_init(1, 6, 4);
    int p1 = veh_init(0, 1, 6, 4);
    int p2 = veh_init(0, 1, 2, 2);
    CHECK(pb >= 0 && p1 >= 0 && p2 >= 0);
    Factions[1].energy_credits = 10000;
    part_upgrade(1, 64, 0);
    CHECK_EQ(Vehs[p1].unit_id, 64);
    CHECK_EQ(Vehs[p2].unit_id, 0);
}

static void test_prune_and_designs() {
    setup_rules();
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    make_proto(3 * 64, CHS_INFANTRY, 0, 0, 0, REC_FUSION); // better reactor, cheaper cost? compare
    Units[3 * 64].cost = Units[0].cost;
    prune_protos(3, 3 * 64, 0);
    CHECK(Units[0].obsolete_factions & (1 << 3));
    CHECK(!(Units[0].obsolete_factions & (1 << 1)));
    // Humans keep obsolete designs unless they asked for automatic pruning
    make_proto(64, CHS_INFANTRY, 0, 0, 0, REC_FUSION);
    Units[64].cost = Units[0].cost;
    prune_protos(1, 64, 0);
    CHECK(!(Units[0].obsolete_factions & (1 << 1)));
    *GameMorePreferences |= MPREF_BSC_AUTO_PRUNE_OBS_VEH;
    prune_protos(1, 64, 0);
    CHECK(Units[0].obsolete_factions & (1 << 1));
    // upgrade_any_prototypes upgrades units of the AI to the flagged design
    setup_rules();
    make_proto(0, CHS_INFANTRY, 0, 0, 0, REC_FISSION);
    Units[0].preq_tech = TECH_None;
    make_proto(3 * 64, CHS_INFANTRY, 0, 0, 0, REC_FUSION);
    Units[3 * 64].unit_flags |= UNIT_PROTOTYPED | UNIT_UNK_100;
    Units[3 * 64].cost = Units[0].cost;
    int va = veh_init(0, 3, 2, 2);
    Factions[3].energy_credits = 10000;
    upgrade_any_prototypes(3);
    CHECK_EQ(Vehs[va].unit_id, 3 * 64);
    CHECK(!(Units[3 * 64].unit_flags & UNIT_UNK_100));
    // consider_designs proposes the standard designs once
    setup_rules();
    for (int t = 0; t < 30; t++) grant(3, t);
    consider_designs(3);
    int made = MaxProtoFactionNum - free_proto_slots(3);
    CHECK(made >= 4);
    bool has_def = false, has_atk = false, has_former = false;
    VehWeapon bw = best_weapon(3);
    VehArmor ba = best_armor(3, 0);
    for (int i = 0; i < 64; i++) {
        UNIT* u = &Units[3 * 64 + i];
        if (!(u->unit_flags & UNIT_ACTIVE)) continue;
        has_def |= u->chassis_id == CHS_INFANTRY && u->weapon_id == WPN_HAND_WEAPONS && u->armor_id == ba;
        has_atk |= u->weapon_id == bw && u->armor_id == ARM_NO_ARMOR && Chassis[u->chassis_id].speed > 1;
        has_former |= u->weapon_id == WPN_TERRAFORMING_UNIT;
    }
    CHECK(has_def);
    CHECK(has_atk);
    CHECK(has_former);
    consider_designs(3);
    CHECK_EQ(MaxProtoFactionNum - free_proto_slots(3), made);
    // Reactor technology proposes upgraded designs for the AI
    setup_rules();
    make_proto(3 * 64, CHS_INFANTRY, 0, 1, 0, REC_FISSION);
    Units[3 * 64].unit_flags |= UNIT_PROTOTYPED;
    grant(3, 0); grant(3, 34);
    tech_achieved(3, 41, 0, 0); // fusion reactor tech (needs 34)
    bool fusion = false;
    for (int i = 0; i < 64; i++) {
        UNIT* u = &Units[3 * 64 + i];
        fusion |= (u->unit_flags & UNIT_ACTIVE) && u->armor_id == 1 && u->reactor_id == REC_FUSION
            && u->chassis_id == CHS_INFANTRY && u->weapon_id == 0;
    }
    CHECK(fusion);
    // Prototype trade: tech_id 97 + unit
    setup_rules();
    make_proto(4 * 64, CHS_SPEEDER, 2, 1, 0, REC_FISSION);
    strcpy(Units[4 * 64].name, "Traded");
    tech_achieved(2, 97 + 4 * 64, 4, 0);
    bool traded = false;
    for (int i = 0; i < 64; i++) {
        UNIT* u = &Units[2 * 64 + i];
        traded |= (u->unit_flags & UNIT_ACTIVE) && !strcmp(u->name, "Traded")
            && (u->unit_flags & UNIT_PROTOTYPED);
    }
    CHECK(traded);
    // Diplomacy scan of another faction's prototypes
    setup_rules();
    make_proto(4 * 64, CHS_SPEEDER, 0, 0, 0, REC_FISSION);
    Units[4 * 64].unit_flags |= UNIT_PROTOTYPED;
    make_proto(4 * 64 + 1, CHS_INFANTRY, 5, 0, 0, REC_FISSION);
    Units[4 * 64 + 1].unit_flags |= UNIT_PROTOTYPED;
    scan_prototypes(2, 4);
    int ids[8];
    CHECK_EQ(scanned_prototypes(ids, 8), 0); // speeder chassis tech missing
    grant(2, 1);
    scan_prototypes(2, 4);
    CHECK_EQ(scanned_prototypes(ids, 8), 1);
    CHECK_EQ(ids[0], 4 * 64);
    propose_proto(2, CHS_SPEEDER, 0, 0, 0, REC_FISSION, -1, nullptr);
    scan_prototypes(2, 4);
    CHECK_EQ(scanned_prototypes(ids, 8), 0);
}

static void test_find_proto() {
    setup_rules();
    for (int i = 0; i < 4; i++) {
        make_proto(i, CHS_INFANTRY, i, i == 3 ? 3 : 0, 0, REC_FISSION);
        Units[i].preq_tech = TECH_None;
    }
    make_proto(4, CHS_INFANTRY, WPN_TERRAFORMING_UNIT, 0, 0, REC_FISSION);
    Units[4].preq_tech = TECH_None;
    int b = base_init(3, 6, 4);
    CHECK(b >= 0);
    Bases[b].governor_flags = GOV_MAY_PROD_PROTOTYPE;
    int id = find_proto(b, TRFLAG_LAND, WMODE_COMBAT, false);
    CHECK(id >= 0 && id < 4);
    CHECK(Units[id].offense_value() > 0);
    CHECK_EQ(find_proto(b, TRFLAG_LAND, WMODE_TERRAFORM, false), 4);
    CHECK(find_proto(b, TRFLAG_SEA, WMODE_COMBAT, false) < 0);
    CHECK(find_proto(99, TRFLAG_LAND, WMODE_COMBAT, false) < 0);
    CHECK(proto_offense(3) == Weapon[3].offense_value);
    CHECK(!unit_is_better(0, 1)); // the stronger unit costs more
    CHECK(unit_is_better(0, 0));
}

/* ---- Oracle modes ---- */

static int run_protocost(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return 2;
    }
    setup_rules();
    char kind[16];
    while (fscanf(fp, "%15s", kind) == 1) {
        if (!strcmp(kind, "C")) { // chassis: i speed triad missile cargo cost
            int i, sp, tr, mi, ca, co;
            if (fscanf(fp, "%d %d %d %d %d %d", &i, &sp, &tr, &mi, &ca, &co) != 6) return 3;
            Chassis[i].speed = (uint8_t)sp; Chassis[i].triad = (uint8_t)tr;
            Chassis[i].missile = (uint8_t)mi; Chassis[i].cargo = (uint8_t)ca; Chassis[i].cost = (uint8_t)co;
        } else if (!strcmp(kind, "W")) { // weapon: i offense mode cost
            int i, o, m, co;
            if (fscanf(fp, "%d %d %d %d", &i, &o, &m, &co) != 4) return 3;
            Weapon[i].offense_value = (int8_t)o; Weapon[i].mode = (uint8_t)m; Weapon[i].cost = (uint8_t)co;
        } else if (!strcmp(kind, "A")) { // armor: i defense cost
            int i, d, co;
            if (fscanf(fp, "%d %d %d", &i, &d, &co) != 3) return 3;
            Armor[i].defense_value = (int8_t)d; Armor[i].cost = (uint8_t)co;
        } else if (!strcmp(kind, "B")) { // ability: i cost flags
            int i, co, fl;
            if (fscanf(fp, "%d %d %d", &i, &co, &fl) != 3) return 3;
            Ability[i].cost = co; Ability[i].flags = fl;
        } else if (!strcmp(kind, "Q")) { // cost query: c w a mask reactor
            int c, w, a, r;
            unsigned mask;
            if (fscanf(fp, "%d %d %d %u %d", &c, &w, &a, &mask, &r) != 5) return 3;
            printf("%d\n", proto_cost(c, w, a, (int)mask, r));
        } else if (!strcmp(kind, "D")) { // derive query (basic slot): c w a mask reactor
            int c, w, a, r;
            unsigned mask;
            if (fscanf(fp, "%d %d %d %u %d", &c, &w, &a, &mask, &r) != 5) return 3;
            make_proto(5, c, w, a, (int)mask, r);
            printf("%d %d %d %d\n", Units[5].cost, Units[5].carry_capacity, Units[5].plan, Units[5].group_id);
        } else {
            return 3;
        }
    }
    fclose(fp);
    return 0;
}

static int run_techrate(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return 2;
    }
    setup_rules();
    int w[25];
    for (;;) {
        int n = 0;
        for (; n < 25; n++) {
            if (fscanf(fp, "%d", &w[n]) != 1) break;
        }
        if (n == 0) break;
        if (n != 25) return 3;
        int faction = w[0];
        for (int i = 1; i < MaxPlayerNum; i++) {
            set_human(i, i == faction);
            Factions[i].tech_cost = -1;
        }
        Factions[faction].diff_level = w[1];
        *CurrentTurn = w[2];
        Rules->tech_discovery_rate = w[3];
        MFactions[faction].rule_techcost = w[4];
        Factions[faction].SE_research_base = w[5];
        int root = 0;
        while ((long long)(root + 1) * (root + 1) <= w[6]) root++;
        *MapAreaSqRoot = root;
        for (int i = 0; i < 8; i++) {
            Factions[i].tech_ranking = i ? w[7 + i] : 0;
            Factions[i].earned_techs_saved = i ? w[15 + i] : 0;
        }
        Factions[faction].unk_27 = w[23];
        if (w[24]) *GameRules |= RULES_TECH_STAGNATION; else *GameRules &= ~RULES_TECH_STAGNATION;
        printf("%d %d\n", tech_rate(faction), mod_tech_rate(faction));
    }
    fclose(fp);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && !strcmp(argv[1], "protocost")) {
        return run_protocost(argv[2]);
    }
    if (argc >= 3 && !strcmp(argv[1], "techrate")) {
        return run_techrate(argv[2]);
    }
    if (argc < 2 || strcmp(argv[1], "unit")) {
        fprintf(stderr, "usage: wp5_test unit | protocost FILE | techrate FILE\n");
        return 2;
    }
    test_tech_avail();
    test_tech_effects();
    test_research_pool_and_pick();
    test_research_change();
    test_tech_rate();
    test_tech_achieved();
    test_tech_achieved_propagation();
    test_tech_advance_steal();
    test_social();
    test_social_change();
    test_make_and_propose();
    test_names();
    test_costs_and_upgrades();
    test_prune_and_designs();
    test_find_proto();
    g_state.reset();
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("%d unit checks passed\n", checks);
    return 0;
}
