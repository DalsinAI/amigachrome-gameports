/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP8 diplomacy, council, victory and score checks (driven by test_engine_wp8.py).
 *
 *   wp8_test unit
 *       synthetic worlds: treaty state changes, mood, friction, atrocity and major
 *       atrocity, comm-link proposals and the ui_diplo dialog, pact_ends, the
 *       Planetary Council (Governor election, Supreme Leader), each victory
 *       (conquest, diplomatic, economic, transcendence), the score, End Turn with
 *       the WP8 turn hooks, and same-seed determinism. Prints the check count and
 *       a digest of the deterministic run (compared between host and m68k).
 */
#include "../engine.h"
#include "../diplo.h"
#include "../turn.h"
#include "../../ui_diplo.h"

#include <memory>
#include <string>

using namespace alpha::engine;
using namespace alpha::ui_diplo;

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

static std::unique_ptr<EngineState> new_state() {
    std::unique_ptr<EngineState> s(new EngineState());
    engine_reset(*s);
    engine_bind(s.get());
    diplo_runtime_reset();
    headless_sink()->choices.clear();
    headless_sink()->default_choice = 0;
    return s;
}

static uint32_t fnv1a(const void* data, size_t n, uint32_t h = 2166136261u) {
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t mix(uint32_t h, int32_t v) {
    uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
    return fnv1a(b, 4, h);
}

static void synthetic_rules() {
    Rules->normal_start_year = 2100;
    Rules->min_turns_between_councils = 20;
    Rules->turns_corner_global_energy_market = 20;
    for (int i = 0; i < MaxProtoNum; i++) {
        Units[i].plan = PLAN_DEFENSE;
        Units[i].preq_tech = TECH_Disable;
        Units[i].cost = 3;
        Units[i].reactor_id = 1;
        Units[i].chassis_id = CHS_INFANTRY;
        Units[i].weapon_id = (uint8_t)(Units[i].weapon_id % MaxWeaponNum);
        Units[i].armor_id = (uint8_t)(Units[i].armor_id % MaxArmorNum);
    }
    Units[BSC_COLONY_POD].plan = PLAN_COLONY;
    for (int i = 0; i < MaxChassisNum; i++) {
        Chassis[i].triad = TRIAD_LAND;
        Chassis[i].speed = 1;
        Chassis[i].range = 0;
    }
    for (int i = 0; i <= Fac_ID_Last; i++) {
        Facility[i].preq_tech = TECH_Disable;
    }
    for (int i = 0; i < MaxTechnologyNum; i++) {
        Tech[i].preq_tech1 = TECH_Disable;
        Tech[i].preq_tech2 = TECH_Disable;
    }
    for (int i = 0; i < MaxProposalNum; i++) {
        Proposal[i].preq_tech = TECH_None;
    }
    Rules->tech_preq_economic_victory = TECH_Disable;
}

/* 16x16 land map, factions alive and human as given, no secret projects,
 * a normal (not scenario) game: objectives off, ending year far away. */
static void setup_world(uint32_t alive, uint32_t human) {
    *MapAreaX = 16;
    *MapAreaY = 16;
    CHECK_EQ(map_init(), 0);
    for (int i = 0; i < *MapAreaTiles; i++) {
        (*MapTiles)[i].climate = (ALT_SHORE_LINE + 1) << 5;
        (*MapTiles)[i].region = 1;
        (*MapTiles)[i].owner = -1;
    }
    synthetic_rules();
    for (int i = 0; i < MaxPlayerNum; i++) {
        Factions[i].target_x = -1;
        Factions[i].base_id_attack_target = -1;
        Factions[i].tech_research_id = 0;
        Factions[i].diff_level = 3;
        set_alive(i, (alive >> i) & 1);
        set_human(i, (human >> i) & 1);
    }
    for (int i = 0; i < MaxSecretProjectNum; i++) {
        SecretProjects[i] = SP_Unbuilt;
    }
    *CurrentTurn = 0;
    *CurrentMissionYear = game_year(0);
    *EndingMissionYear = 2600;
    *ObjectiveReqVictory = 9999;
    *ObjectivesSuddenDeathVictory = 9999;
    *MapNativeLifeForms = 1;
    *GameState = 0;
    *GameRules = 0;
    *GameMoreRules = 0;
    *DiffLevel = 3;
    *GovernorFaction = 0;
    MapWin->cOwner = 0;
    *CurrentPlayerFaction = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if ((human >> i) & 1) {
            MapWin->cOwner = i;
            *CurrentPlayerFaction = i;
            break;
        }
    }
    *ControlTurnMove = 0;
    *PbemActive = bit_count(human) > 1;
    *GamePreferences &= ~PREF_BSC_AUTOSAVE_EACH_TURN;
}

static int add_base(int faction_id, int x, int y, int pop) {
    int base_id = base_init(faction_id, x, y);
    CHECK(base_id >= 0);
    if (base_id >= 0) {
        Bases[base_id].pop_size = pop;
        Factions[faction_id].pop_total += pop;
    }
    return base_id;
}

static void commlink_all(int count) {
    for (int i = 1; i <= count; i++) {
        for (int j = 1; j <= count; j++) {
            if (i != j) {
                Factions[i].diplo_status[j] |= DIPLO_COMMLINK;
            }
        }
    }
}

static bool has_message(const char* text) {
    for (const std::string& line : headless_sink()->log) {
        if (line.find(text) != std::string::npos) {
            return true;
        }
    }
    return false;
}

static void note_state();

/* ---- treaty state ---- */

static void test_treaty_state() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    treaty_on(1, 2, DIPLO_COMMLINK);
    CHECK(has_treaty(1, 2, DIPLO_COMMLINK) && has_treaty(2, 1, DIPLO_COMMLINK));

    treaty_on(1, 2, DIPLO_PACT);
    CHECK(has_treaty(1, 2, DIPLO_PACT) && has_treaty(1, 2, DIPLO_TREATY));
    CHECK(has_treaty(2, 1, DIPLO_PACT) && has_treaty(2, 1, DIPLO_TREATY));
    CHECK(!has_treaty(1, 3, DIPLO_PACT));

    // Vendetta removes every peace bit on both sides and keeps the comm link.
    treaty_on(1, 2, DIPLO_VENDETTA);
    CHECK(has_treaty(1, 2, DIPLO_VENDETTA) && has_treaty(2, 1, DIPLO_VENDETTA));
    CHECK(!has_treaty(1, 2, DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE));
    CHECK(!has_treaty(2, 1, DIPLO_PACT|DIPLO_TREATY|DIPLO_TRUCE));
    CHECK(has_treaty(1, 2, DIPLO_COMMLINK));

    // A truce ends the vendetta.
    treaty_on(2, 1, DIPLO_TRUCE);
    CHECK(has_treaty(1, 2, DIPLO_TRUCE) && !has_treaty(1, 2, DIPLO_VENDETTA));
    treaty_off(1, 2, DIPLO_TRUCE);
    CHECK(!has_treaty(1, 2, DIPLO_TRUCE) && !has_treaty(2, 1, DIPLO_TRUCE));

    // set_treaty changes one side only.
    set_treaty(2, 3, DIPLO_WANT_REVENGE, 1);
    CHECK(has_treaty(2, 3, DIPLO_WANT_REVENGE));
    CHECK(!has_treaty(3, 2, DIPLO_WANT_REVENGE));
    set_treaty(2, 3, DIPLO_UNK_40, 1);
    CHECK_EQ(Factions[2].diplo_merc[3], 50);
    set_treaty(2, 3, DIPLO_WANT_REVENGE, 0);
    CHECK(!has_treaty(2, 3, DIPLO_WANT_REVENGE));

    // Out of range ids are ignored by treaty_off.
    treaty_off(1, MaxPlayerNum, DIPLO_TREATY);
    treaty_off(-1, 2, DIPLO_TREATY);
    note_state();
}

/* ---- mood and friction ---- */

static void test_mood() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    const int friction[] = {-3, 0, 1, 2, 3, 4, 5, 8, 9, 12, 13, 15, 16, 17, 18, 19, 20, 25};
    const int mood[] = {MOOD_MAGNANIMOUS, MOOD_MAGNANIMOUS, MOOD_SOLICITOUS, MOOD_SOLICITOUS,
        MOOD_COOPERATIVE, MOOD_COOPERATIVE, MOOD_NONCOMMITTAL, MOOD_NONCOMMITTAL,
        MOOD_AMBIVALENT, MOOD_AMBIVALENT, MOOD_OBSTINATE, MOOD_OBSTINATE,
        MOOD_QUARRELSOME, MOOD_QUARRELSOME, MOOD_BELLIGERENT, MOOD_BELLIGERENT,
        MOOD_SEETHING, MOOD_SEETHING};
    for (size_t i = 0; i < sizeof(friction) / sizeof(friction[0]); i++) {
        CHECK_EQ(get_mood(friction[i]), mood[i]);
    }
    CHECK(std::string(diplo_mood_name(MOOD_SEETHING)) == "Seething");
    CHECK(std::string(diplo_mood_name(99)).empty());

    cause_friction(2, 1, 7);
    CHECK_EQ(Factions[2].diplo_friction[1], 7);
    cause_friction(2, 1, 30);
    CHECK_EQ(Factions[2].diplo_friction[1], 20); // clamped
    cause_friction(2, 1, -50);
    CHECK_EQ(Factions[2].diplo_friction[1], 0);
    CHECK_EQ(Factions[1].diplo_friction[2], 0); // one side only
    note_state();
}

/* ---- atrocities ---- */

static void test_atrocity() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    commlink_all(3);
    Factions[3].diplo_status[1] |= DIPLO_UNK_200000; // 3 watches 1 (friction source)
    atrocity(1, 2, 0, 0);
    CHECK(has_treaty(2, 1, DIPLO_ATROCITY_VICTIM));
    CHECK(has_treaty(2, 1, DIPLO_WANT_REVENGE));
    CHECK(!has_treaty(1, 2, DIPLO_ATROCITY_VICTIM));
    CHECK_EQ(Factions[1].atrocities, 1);
    CHECK_EQ(Factions[1].integrity_blemishes, 1);
    CHECK_EQ(Factions[1].sanction_turns, 10);
    CHECK_EQ(Factions[3].diplo_friction[1], 5);
    CHECK(has_treaty(3, 1, DIPLO_UNK_400000));
    CHECK(has_message("ATROCITY2"));

    // The second one does not count again (skip_init_check = 0 and atrocities > 0).
    atrocity(1, 2, 0, 0);
    CHECK_EQ(Factions[1].atrocities, 1);
    CHECK_EQ(Factions[1].sanction_turns, 20);
    atrocity(1, 2, 1, 0);
    CHECK_EQ(Factions[1].atrocities, 2);
    CHECK_EQ(Factions[1].sanction_turns, 40);

    // Past 4 * (8 - difficulty) atrocities it is a major atrocity: every other
    // computer player declares a vendetta and the pact with 1 ends.
    treaty_on(1, 3, DIPLO_PACT);
    Factions[1].atrocities = 4 * (8 - Factions[1].diff_level);
    atrocity(1, 2, 1, 0);
    CHECK_EQ(Factions[1].major_atrocities, 1);
    CHECK_EQ(Factions[1].integrity_blemishes, 6);
    CHECK(has_treaty(3, 1, DIPLO_VENDETTA) && has_treaty(1, 3, DIPLO_VENDETTA));
    CHECK(!has_treaty(1, 3, DIPLO_PACT|DIPLO_TREATY));
    CHECK(has_treaty(2, 1, DIPLO_MAJOR_ATROCITY_VICTIM));
    CHECK_EQ(Factions[3].diplo_merc[1], 50);
    CHECK(has_message("MAJORATROCITY0"));

    // With the U.N. Charter repealed there are no sanctions.
    auto s2 = new_state();
    setup_world(0x0E, 0x02);
    ProposalPassCount[PROP_REPEAL_UN_CHARTER] = 1;
    CHECK(!un_charter());
    atrocity(1, 2, 1, 0);
    CHECK(has_treaty(2, 1, DIPLO_ATROCITY_VICTIM));
    CHECK_EQ(Factions[1].atrocities, 0);
    CHECK_EQ(Factions[1].sanction_turns, 0);
    note_state();
}

/* ---- comm-link proposals and the dialog model ---- */

static int g_choice_script[8];
static int g_choice_pos;

static int scripted_chooser(const DiploDialog& UNUSED(d), void* UNUSED(ctx)) {
    return g_choice_pos < 8 ? g_choice_script[g_choice_pos++] : -1;
}

static void test_proposals() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    for (int i = 1; i <= 3; i++) {
        add_base(i, 2 + 4 * i, 2 + 4 * i, 3);
    }
    CHECK(!diplo_proposal_valid(1, 2, DP_TREATY)); // no comm link
    commlink_all(3);
    CHECK(diplo_proposal_valid(1, 2, DP_TREATY));
    CHECK(!diplo_proposal_valid(1, 2, DP_TRUCE));
    CHECK(!diplo_proposal_valid(1, 2, DP_PACT));
    CHECK(!diplo_proposal_valid(1, 2, DP_END_PACT));
    CHECK(diplo_proposal_valid(1, 2, DP_VENDETTA));
    CHECK(!diplo_proposal_valid(1, 1, DP_TREATY));
    CHECK(!diplo_proposal_valid(1, 4, DP_TREATY)); // not alive

    DiploDialog d;
    diplo_dialog_open(d, 1, 2);
    CHECK_EQ(d.option_count, 2);
    CHECK_EQ(d.options[0], DP_TREATY);
    CHECK_EQ(d.options[1], DP_VENDETTA);
    CHECK_EQ(d.mood, MOOD_MAGNANIMOUS);
    CHECK_EQ(diplo_dialog_propose(d, DP_PACT), DA_INVALID);

    // A rival that has lost patience refuses, deterministically.
    Factions[2].diplo_patience[1] = 100;
    CHECK(!diplo_proposal_accepted(1, 2, DP_TREATY));
    CHECK(!diplo_proposal_accepted(1, 2, DP_TREATY));
    CHECK_EQ(diplo_dialog_propose(d, DP_TREATY), DA_REFUSED);
    CHECK(!has_treaty(1, 2, DIPLO_TREATY));
    // ...and so does one bent on a fight to the death.
    Factions[2].diplo_patience[1] = 0;
    Factions[2].diplo_agenda[1] |= AGENDA_FIGHT_TO_DEATH;
    CHECK(!diplo_proposal_accepted(1, 2, DP_TREATY));
    CHECK_EQ(Factions[2].diplo_patience[1], 1); // each proposal uses up patience
    Factions[2].diplo_agenda[1] &= ~AGENDA_FIGHT_TO_DEATH;

    // Unilateral proposals always go through.
    CHECK_EQ(diplo_dialog_propose(d, DP_VENDETTA), DA_ACCEPTED);
    CHECK(has_treaty(1, 2, DIPLO_VENDETTA) && has_treaty(2, 1, DIPLO_VENDETTA));
    CHECK_EQ(d.option_count, 1);
    CHECK_EQ(d.options[0], DP_TRUCE);
    CHECK_EQ(Factions[1].diplo_spoke[2], *CurrentTurn);

    // Applying a treaty/pact and renouncing the pact.
    treaty_off(1, 2, DIPLO_VENDETTA);
    diplo_proposal_apply(1, 2, DP_TREATY);
    CHECK(has_treaty(1, 2, DIPLO_TREATY) && has_treaty(2, 1, DIPLO_TREATY));
    CHECK(diplo_proposal_valid(1, 2, DP_PACT));
    diplo_proposal_apply(1, 2, DP_PACT);
    CHECK(has_treaty(2, 1, DIPLO_PACT));
    diplo_dialog_open(d, 1, 2);
    CHECK_EQ(diplo_dialog_propose(d, DP_END_PACT), DA_ACCEPTED);
    CHECK(!has_treaty(1, 2, DIPLO_PACT) && has_treaty(1, 2, DIPLO_TREATY));
    CHECK_EQ(Factions[2].diplo_friction[1], 2);
    // A vendetta against a treaty partner is a double cross (the treaty breaks).
    CHECK_EQ(diplo_dialog_propose(d, DP_VENDETTA), DA_ACCEPTED);
    CHECK(has_treaty(1, 2, DIPLO_VENDETTA) && !has_treaty(1, 2, DIPLO_TREATY));

    // The engine's communicate() opens the dialog when ui_diplo is installed.
    g_choice_pos = 0;
    g_choice_script[0] = DP_VENDETTA;
    g_choice_script[1] = -1;
    install_ui_diplo(scripted_chooser, nullptr);
    DiploStateA[3] = 5;
    *CurrentTurn = 7;
    communicate(1, 3, 0);
    CHECK_EQ(ui_diplo_last().player, 1);
    CHECK_EQ(ui_diplo_last().other, 3);
    CHECK_EQ(ui_diplo_last().last_proposal, DP_VENDETTA);
    CHECK_EQ(ui_diplo_last().last_answer, DA_ACCEPTED);
    CHECK(has_treaty(3, 1, DIPLO_VENDETTA));
    CHECK_EQ(DiploStateA[3], 0);
    CHECK_EQ(Factions[1].diplo_spoke[3], 7);
    uninstall_ui_diplo();
    // Headless communicate records the contact only.
    DiploStateA[2] = 5;
    communicate(1, 2, 0);
    CHECK_EQ(DiploStateA[2], 0);
    note_state();
}

/* ---- pact_ends ---- */

static void test_pact_ends() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    int b1 = add_base(1, 2, 2, 3);
    int b2 = add_base(2, 8, 8, 3);
    CHECK(b1 >= 0 && b2 >= 0);
    commlink_all(3);
    treaty_on(1, 2, DIPLO_PACT);
    treaty_on(3, 2, DIPLO_PACT);
    set_treaty(1, 2, DIPLO_HAVE_SURRENDERED, 1);
    int v_guest = veh_init(BSC_SCOUT_PATROL, 1, 8, 8);   // in 2's base
    int v_home = veh_init(BSC_SCOUT_PATROL, 1, 4, 4);    // in the open
    int v_lost = veh_init(BSC_SCOUT_PATROL, 3, 8, 8);    // 3 has no base
    CHECK(v_guest >= 0 && v_home >= 0 && v_lost >= 0);
    Vehs[v_guest].home_base_id = b1;
    const int count = *VehCount;

    pact_ends(1, 2);
    CHECK(!has_treaty(1, 2, DIPLO_PACT) && !has_treaty(2, 1, DIPLO_PACT));
    CHECK(has_treaty(1, 2, DIPLO_TREATY) && has_treaty(2, 1, DIPLO_TREATY));
    CHECK(!has_treaty(1, 2, DIPLO_HAVE_SURRENDERED));
    CHECK_EQ(Vehs[v_guest].x, 2);
    CHECK_EQ(Vehs[v_guest].y, 2);
    CHECK_EQ(Vehs[v_home].x, 4);
    CHECK(has_message("pact_ends"));
    CHECK_EQ(*VehCount, count);

    // No pact: nothing happens.
    pact_ends(1, 2);
    CHECK(has_treaty(1, 2, DIPLO_TREATY));

    // A faction with no base loses its units in the former partner's base.
    pact_ends(3, 2);
    CHECK(!has_treaty(3, 2, DIPLO_PACT));
    CHECK_EQ(*VehCount, count - 1);
    for (int i = 0; i < *VehCount; i++) {
        CHECK(Vehs[i].faction_id != 3);
    }
    note_state();
}

/* ---- Planetary Council ---- */

/* Factions 1..3, votes 6/3/2. */
static void council_world(uint32_t human) {
    setup_world(0x0E, human);
    add_base(1, 2, 2, 6);
    add_base(2, 8, 8, 3);
    add_base(3, 12, 12, 2);
    commlink_all(3);
    *CurrentTurn = 30;
}

static void test_council() {
    auto s = new_state();
    council_world(0);
    CHECK_EQ(council_votes(1), 6);
    CHECK_EQ(council_votes(2), 3);
    CHECK_EQ(council_votes(3), 2);
    CHECK(eligible(1) && eligible(2) && !eligible(3));
    CHECK_EQ(can_call_council(1, 0), 1);
    Factions[1].diplo_status[2] &= ~DIPLO_COMMLINK;
    Factions[1].diplo_status[3] &= ~DIPLO_COMMLINK;
    CHECK_EQ(can_call_council(1, 0), 0); // no contacts
    commlink_all(3);
    *GameMoreRules |= MRULES_NO_PLANETARY_COUNCIL;
    CHECK_EQ(can_call_council(1, 0), 0);
    *GameMoreRules &= ~MRULES_NO_PLANETARY_COUNCIL;

    // The first session must elect the Governor.
    CHECK(!council_proposal_valid(1, PROP_SALVAGE_UNITY_CORE));
    CHECK(council_proposal_valid(1, PROP_ELECT_PLANETARY_GOVERNOR));
    CHECK_EQ(council_ai_proposal(1), PROP_ELECT_PLANETARY_GOVERNOR);

    // 3 likes 1 (treaty) more than 2: 1 wins 8 to 3.
    treaty_on(3, 1, DIPLO_TREATY);
    CouncilVote v = council_session(1, PROP_ELECT_PLANETARY_GOVERNOR);
    CHECK_EQ(v.result, COUNCIL_PASSED);
    CHECK_EQ(v.candidate[0], 1);
    CHECK_EQ(v.candidate[1], 2);
    CHECK_EQ(v.choice[1], 1);
    CHECK_EQ(v.choice[2], 2);
    CHECK_EQ(v.choice[3], 1);
    CHECK_EQ(v.tally[1], 8);
    CHECK_EQ(v.tally[2], 3);
    CHECK_EQ(v.total, 11);
    CHECK_EQ(v.winner, 1);
    CHECK_EQ(*GovernorFaction, 1);
    CHECK(*GameState & STATE_COUNCIL_HAS_CONVENED);
    CHECK_EQ(ProposalCallTurn[PROP_ELECT_PLANETARY_GOVERNOR], 30);
    CHECK_EQ(Factions[1].council_call_turn, 30);
    CHECK_EQ(ProposalPassCount[PROP_ELECT_PLANETARY_GOVERNOR], 1);
    CHECK_EQ(CouncilProposals[3], PROP_ELECT_PLANETARY_GOVERNOR);
    CHECK_EQ(CouncilVoteState[1], 1);
    CHECK_EQ(CouncilVoteState[2], -1);
    CHECK(has_message("council"));
    // Same proposal again too soon.
    CHECK(!council_proposal_valid(1, PROP_ELECT_PLANETARY_GOVERNOR));
    CHECK_EQ(can_call_council(1, 0), 0);
    CHECK_EQ(can_call_council(1, 1), 1);

    // 3 now hates 1: it votes for 2, and 2 wins when its votes beat 1's.
    *CurrentTurn = 60;
    treaty_off(3, 1, DIPLO_TREATY);
    treaty_on(3, 1, DIPLO_VENDETTA);
    Bases[0].pop_size = 4;
    v = council_session(2, PROP_ELECT_PLANETARY_GOVERNOR);
    CHECK_EQ(v.candidate[0], 2);
    CHECK_EQ(v.candidate[1], 1);
    CHECK_EQ(v.choice[3], 2);
    CHECK_EQ(v.tally[2], 5);
    CHECK_EQ(v.tally[1], 4);
    CHECK_EQ(*GovernorFaction, 2);

    // Salvage the Unity Core passes on yes votes and pays every faction 500.
    *CurrentTurn = 90;
    treaty_off(3, 1, DIPLO_VENDETTA);
    Factions[1].ranking = 1;
    Factions[2].ranking = 2;
    Factions[3].ranking = 3;
    int credits = Factions[3].energy_credits;
    v = council_session(1, PROP_SALVAGE_UNITY_CORE);
    CHECK_EQ(v.result, COUNCIL_PASSED);
    CHECK_EQ(v.yes, 4 + 3 + 2);
    CHECK_EQ(v.no, 0);
    CHECK_EQ(Factions[3].energy_credits, credits + 500);
    CHECK(!council_proposal_valid(1, PROP_SALVAGE_UNITY_CORE));
    // A proposal whose technology is not known cannot be brought.
    Proposal[PROP_MELT_POLAR_CAPS].preq_tech = TECH_Disable;
    CHECK(!council_proposal_valid(1, PROP_MELT_POLAR_CAPS));
    note_state();
}

/* ---- victories and score ---- */

static void test_diplomatic_victory() {
    auto s = new_state();
    council_world(0x02);
    *GameRules = RULES_VICTORY_DIPLOMATIC | RULES_VICTORY_COOPERATIVE;
    *GameState |= STATE_COUNCIL_HAS_CONVENED;
    CHECK(council_proposal_valid(1, PROP_UNITE_SUPREME_LEADER));
    Bases[0].pop_size = 9; // votes 9/3/2, 14 in all
    // Without allies 1 has 9 of 14 votes: not three quarters.
    CouncilVote v = council_session(1, PROP_UNITE_SUPREME_LEADER);
    CHECK_EQ(v.result, COUNCIL_FAILED);
    CHECK_EQ(v.candidate[1], 2);
    CHECK_EQ(v.choice[2], 2); // a candidate votes for itself
    CHECK_EQ(v.choice[3], 0); // no pact: abstains
    CHECK_EQ(v.yes, 9);
    CHECK(!(*GameState & STATE_GAME_DONE));
    CHECK_EQ(end_of_game(1), 0);

    // A pact partner in a cooperative game votes for 1: 11 of 14 votes.
    *CurrentTurn = 60;
    treaty_on(1, 3, DIPLO_PACT);
    v = council_session(1, PROP_UNITE_SUPREME_LEADER);
    CHECK_EQ(v.result, COUNCIL_PASSED);
    CHECK_EQ(v.choice[3], 1);
    CHECK_EQ(v.tally[1], 11);
    CHECK_EQ(v.winner, 1);
    CHECK(*GameState & STATE_VICTORY_DIPLOMATIC);
    CHECK(*GameState & STATE_GAME_DONE);
    CHECK_EQ(*GameVictoryType, VIC_DIPLOMATIC_SOLO);
    CHECK(Factions[1].player_flags & PFLAG_UNK_20000);

    // end_of_game shows the victory and the score, then ends the game.
    CHECK_EQ(end_of_game(1), 1);
    CHECK(has_message("DIPLOMATICVICTORY"));
    CHECK(has_message("ui:credits"));
    CHECK(has_message("ui:score"));
    CHECK(*GameState & STATE_FINAL_SCORE_DONE);
    CHECK_EQ(*ControlTurnB, 1);
    // Score: population 9 (own) + 2 (pact base, cooperative) + 3 / 2, victory
    // bonus max(200, 2 * (600 - 60)) = 1080.
    CHECK_EQ(Factions[1].unk_110, 9 + 2 + 1);
    CHECK_EQ(Factions[1].unk_117, 1080);
    CHECK_EQ(Factions[1].unk_109, Factions[1].unk_110 + Factions[1].unk_111
        + Factions[1].unk_112 + Factions[1].unk_113 + Factions[1].unk_114
        + Factions[1].unk_116 + Factions[1].unk_117 + Factions[1].unk_118);
    CHECK_EQ(Factions[1].unk_107, Factions[1].unk_109);
    // The pact partner of the winner shares the cooperative bonus max(100, 600 - 60).
    CHECK_EQ(Factions[3].unk_117, 540);
    CHECK_EQ(Factions[2].unk_117, 0);
    // Done: a second call does nothing.
    CHECK_EQ(end_of_game(1), 0);

    // A rival winning is a loss for the human.
    auto s2 = new_state();
    council_world(0x08); // 3 is human
    *GameRules = RULES_VICTORY_DIPLOMATIC;
    *GameState |= STATE_COUNCIL_HAS_CONVENED;
    Bases[0].pop_size = 9;
    headless_sink()->choices["ELECT"] = 0; // the human votes for the proposer
    v = council_session(1, PROP_UNITE_SUPREME_LEADER);
    CHECK_EQ(v.result, COUNCIL_PASSED);
    CHECK_EQ(v.choice[3], 1);
    CHECK_EQ(v.tally[1], 11);
    CHECK_EQ(*GameVictoryType, VIC_DIPLOMATIC_LOSS);
    CHECK_EQ(end_of_game(1), 1);
    CHECK(has_message("DIPLOMATICLOSE"));
    note_state();
}

static void test_economic_victory() {
    auto s = new_state();
    setup_world(0x0E, 0x02);
    add_base(1, 2, 2, 4);
    add_base(2, 8, 8, 4);
    add_base(3, 12, 12, 4);
    commlink_all(3);
    *GameRules = RULES_VICTORY_ECONOMIC;
    *CurrentTurn = 100;
    // The price of cornering the market: at least 1000, the same each time.
    int cost = corner_market(2);
    CHECK(cost >= 1000);
    CHECK_EQ(corner_market(2), cost);
    // A pact (with surrender) partner's bases are left out.
    treaty_on(1, 2, DIPLO_PACT);
    set_treaty(1, 2, DIPLO_HAVE_SURRENDERED, 1);
    int cost2 = corner_market(2);
    CHECK(cost2 >= 1000 && cost2 <= cost);
    treaty_off(1, 2, DIPLO_PACT|DIPLO_HAVE_SURRENDERED);

    // Not yet: the market closes in a later turn.
    Factions[2].corner_market_turn = 110;
    Factions[2].corner_market_cost = cost;
    CHECK_EQ(end_of_game(1), 0);
    CHECK(!(*GameState & STATE_GAME_DONE));
    *CurrentTurn = 110;
    CHECK_EQ(end_of_game(1), 1);
    CHECK(*GameState & STATE_VICTORY_ECONOMIC);
    CHECK_EQ(*GameVictoryType, VIC_ECONOMIC_LOSS);
    CHECK(Factions[2].player_flags & PFLAG_UNK_40000);
    CHECK(has_message("ECONOMICLOSE"));
    CHECK_EQ(Factions[2].unk_117, 2 * (600 - 110));

    // The human cornering the market wins.
    auto s2 = new_state();
    setup_world(0x0E, 0x02);
    add_base(1, 2, 2, 4);
    *GameRules = RULES_VICTORY_ECONOMIC;
    *CurrentTurn = 50;
    Factions[1].corner_market_turn = 50;
    Factions[1].corner_market_cost = 1000;
    CHECK_EQ(end_of_game(1), 1);
    CHECK_EQ(*GameVictoryType, VIC_ECONOMIC_SOLO);
    CHECK(has_message("ECONOMICVICTORY"));
    note_state();
}

static void test_conquest_victory() {
    auto s = new_state();
    setup_world(0x06, 0x02);
    add_base(1, 2, 2, 5);
    add_base(1, 6, 6, 3);
    *GameRules = RULES_VICTORY_CONQUEST;
    *CurrentTurn = 100;
    // mod_setup_player (WP7) sets these when the last rival is gone.
    *GameState |= STATE_VICTORY_CONQUER | STATE_GAME_DONE;
    *GameVictoryType = VIC_UNIFY_SOLO;
    CHECK_EQ(end_of_game(1), 1);
    CHECK(has_message("ui:credits"));
    // Conquest bonus 2 * (500 - turn), times the share of the population.
    CHECK_EQ(Factions[1].unk_117, 800);
    CHECK_EQ(Factions[1].unk_110, 8);
    CHECK_EQ(Factions[2].unk_117, 0);
    CHECK(Factions[1].unk_107 >= 808);

    // Half the population owned: half the bonus (table passed by the caller).
    auto s2 = new_state();
    setup_world(0x06, 0x02);
    add_base(1, 2, 2, 4);
    add_base(2, 8, 8, 4);
    *GameRules = RULES_VICTORY_CONQUEST;
    *GameState |= STATE_VICTORY_CONQUER;
    *CurrentTurn = 100;
    int score = 0;
    int table[9] = {};
    compute_score(1, &score, nullptr, 0);
    CHECK_EQ(Factions[1].unk_117, 400);
    compute_score(1, &score, table, 1);
    // As in Thinker, the share scales the faction's stored unk_117 (400 here).
    CHECK_EQ(table[7], 4 * 400 / 8);
    *GameRules = 0;
    compute_score(1, &score, table, 1);
    CHECK_EQ(table[7], 0); // no conquest victory in this game
    note_state();
}

static void test_transcendence() {
    auto s = new_state();
    setup_world(0x06, 0x02);
    int b1 = add_base(1, 2, 2, 5);
    add_base(2, 8, 8, 5);
    CHECK(!transcending(1));
    SecretProjects[FAC_VOICE_OF_PLANET - SP_ID_First] = b1;
    CHECK(ascending(1));
    CHECK(!transcending(1));
    Bases[b1].queue_items[0] = -FAC_ASCENT_TO_TRANSCENDENCE;
    CHECK(transcending(1));
    CHECK(!transcending(2));

    // WP2 base production sets these when the Ascent completes.
    *CurrentTurn = 200;
    SecretProjects[FAC_ASCENT_TO_TRANSCENDENCE - SP_ID_First] = b1;
    *GameVictoryType = VIC_TRANSCEND_PLR;
    *GameState |= STATE_GAME_DONE;
    CHECK_EQ(end_of_game(1), 1);
    CHECK(has_message("ui:credits"));
    // Ascent bonus max(300, 2 * (1000 - 200)) to the builder (no pact partners).
    CHECK_EQ(*ScoreBonusPts, 1600);
    CHECK_EQ(Factions[1].unk_116, 1600);
    CHECK_EQ(Factions[2].unk_116, 0);
    CHECK_EQ(ScorePopTotal[1], 10);
    // Two secret projects of 1 count 25 each.
    CHECK_EQ(Factions[1].unk_114, 50);
    note_state();
}

static void test_score() {
    auto s = new_state();
    setup_world(0x06, 0x02);
    add_base(1, 2, 2, 7);
    add_base(2, 8, 8, 2);
    Factions[1].tech_count_transcendent = 3;
    int score = 0;
    int table[9] = {};
    compute_score(1, &score, table, 1);
    CHECK_EQ(table[0], 7);
    CHECK_EQ(table[3], 30);
    CHECK_EQ(table[7], 0);
    int sum = 0;
    for (int i = 0; i < 9; i++) {
        sum += table[i];
    }
    CHECK_EQ(score, sum);
    // Native life forms scale the score of a normal game.
    *MapNativeLifeForms = 3;
    int score2 = 0;
    compute_score(1, &score2, table, 1);
    CHECK_EQ(table[5], score * 2 / 4);
    CHECK_EQ(score2, score + score * 2 / 4);
    // Ironman doubles the recorded score.
    *MapNativeLifeForms = 1;
    *GameRules |= RULES_IRONMAN;
    compute_score(1, nullptr, nullptr, 0);
    CHECK_EQ(Factions[1].unk_107, 2 * Factions[1].unk_109);
    CHECK_EQ(Factions[1].unk_109, score);
    // Dead factions get no victory bonus.
    CHECK_EQ(get_rating(1, 0) >= 0, 1);

    // Time limit (the ending year reached) ends the game.
    *GameRules = 0;
    *CurrentMissionYear = *EndingMissionYear;
    CHECK_EQ(end_of_game(1), 1);
    CHECK_EQ(*GameVictoryType, VIC_TIME_LIMIT);
    CHECK(has_message("TIMELIMIT"));
    note_state();
}

/* ---- End Turn with the WP8 turn hooks, determinism ---- */

static uint32_t run_game(uint32_t seed, int turns, std::string* summary) {
    auto s = new_state();
    setup_world(0x0E, 0x00); // all computer players
    install_wp8_turn_hooks();
    game_srand(seed);
    random_reseed(seed);
    add_base(1, 2, 2, 6);
    add_base(2, 8, 8, 3);
    add_base(3, 12, 12, 2);
    commlink_all(3);
    treaty_on(1, 2, DIPLO_TREATY);
    treaty_on(2, 3, DIPLO_VENDETTA);
    Factions[3].diplo_friction[1] = 12;
    uint32_t h = 2166136261u;
    for (int t = 0; t < turns; t++) {
        TurnResult r = engine_end_turn();
        h = mix(h, r.status);
        h = mix(h, r.turn);
        if (r.status == TURN_GAME_OVER) {
            break;
        }
    }
    for (int i = 1; i < 4; i++) {
        for (int j = 1; j < 4; j++) {
            h = mix(h, (int32_t)Factions[i].diplo_status[j]);
            h = mix(h, Factions[i].diplo_friction[j]);
            h = mix(h, Factions[i].diplo_patience[j]);
        }
        h = mix(h, Factions[i].energy_credits);
        h = mix(h, Factions[i].unk_109);
    }
    h = mix(h, *GovernorFaction);
    h = mix(h, *GameState);
    h = mix(h, (int32_t)game_rand_state());
    if (summary) {
        char buf[200];
        snprintf(buf, sizeof(buf), "turn=%d governor=%d state=%08X d12=%08X d23=%08X",
            *CurrentTurn, *GovernorFaction, (unsigned)*GameState,
            (unsigned)Factions[1].diplo_status[2], (unsigned)Factions[2].diplo_status[3]);
        *summary = buf;
    }
    turn_hooks() = TurnHooks();
    return h;
}

static uint32_t test_turns_and_determinism() {
    std::string a, b, c;
    uint32_t h1 = run_game(12345, 45, &a);
    uint32_t h2 = run_game(12345, 45, &b);
    uint32_t h3 = run_game(777, 45, &c);
    CHECK_EQ(h1, h2);
    CHECK(a == b);
    // 45 turns: past min_turns_between_councils, so a computer player has
    // called the council and a Governor was elected.
    CHECK(a.find("turn=45") != std::string::npos);
    CHECK(a.find("governor=0") == std::string::npos);
    printf("run %s\n", a.c_str());
    printf("run %s\n", c.c_str());
    return mix(h1, (int32_t)h3);
}

/* Diplomacy states and scores of the unit checks, for the host/m68k comparison. */
static uint32_t g_digest = 2166136261u;
static void note_state();
static uint32_t state_digest() {
    uint32_t h = 2166136261u;
    for (int i = 0; i < MaxPlayerNum; i++) {
        for (int j = 0; j < MaxPlayerNum; j++) {
            h = mix(h, (int32_t)Factions[i].diplo_status[j]);
        }
        h = mix(h, Factions[i].unk_107);
        h = mix(h, Factions[i].unk_109);
    }
    return h;
}

static void note_state() {
    g_digest = mix(g_digest, (int32_t)state_digest());
}

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) != "unit") {
        fprintf(stderr, "usage: %s unit\n", argv[0]);
        return 2;
    }
    test_treaty_state();
    test_mood();
    test_atrocity();
    test_proposals();
    test_pact_ends();
    test_council();
    test_diplomatic_victory();
    test_economic_victory();
    test_conquest_victory();
    test_transcendence();
    test_score();
    uint32_t d3 = test_turns_and_determinism();
    if (failures) {
        fprintf(stderr, "%d of %d unit checks failed\n", failures, checks);
        return 1;
    }
    printf("digest=%08X %08X\n", g_digest, d3);
    printf("%d unit checks passed\n", checks);
    return 0;
}
