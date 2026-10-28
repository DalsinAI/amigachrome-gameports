/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP1: turn scheduler and End Turn.
 * control_turn, turn_upkeep, faction_upkeep, repair_phase, production_phase,
 * allocate_energy, next_player_turn, do_fungal_towers and random_events are ported
 * from Thinker Mod src/gameturn.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af
 * (MIT; see engine/THINKER-LICENSE), game_year from src/game.cpp. Window, network and
 * password code is left out; popups go to the event sink. Deviations are marked
 * with "Fix:" or "Original:" and listed in ENGINE-WP1.md.
 */
#include "engine.h"
#include "turn.h"

namespace alpha {
namespace engine {

/* Runtime flags of the original executable used only inside the turn code.
 * They are transient (never saved, reset by every control_turn_run call). */
static int s_control_turn_a = 0;  // ControlTurnA: quit/load requested, leave control_turn
static int s_control_upkeep_a = 0; // ControlUpkeepA: 1 while faction_upkeep runs
static int s_energy_credits = 0;  // EnergyCredits: credits before this turn's base upkeep
static int s_skip_tech_screen_a = 0; // SkipTechScreenA
static int s_net_upkeep_state = 0; // NetUpkeepState (network only, always 0)
static int s_dword_93A940 = 0;
static int s_dword_93A950 = 0;

static TurnHooks s_hooks = {};

static int wrap_add(int a, int b) {
    return (int)((uint32_t)a + (uint32_t)b);
}

TurnHooks& turn_hooks() {
    return s_hooks;
}

int __cdecl game_year(int turn) {
    return Rules->normal_start_year + turn;
}

int turn_order_next(int faction_id) {
    for (int i = 1; i <= MaxPlayerNum; i++) {
        int id = (faction_id + i) % MaxPlayerNum;
        if (faction_id < 0) {
            id = i - 1;
        }
        if (is_alive(id)) {
            return id;
        }
    }
    return -1;
}

/*
Original: the human hotseat switch shows the SWITCHING popup, checks the faction
password and refreshes every window. Only its game-state parts are kept here.
*/
static void human_turn_switch(int current_id) {
    parse_says(0, "", -1, -1);
    parse_says(1, get_title(current_id), -1, -1);
    parse_says(2, get_name(current_id), -1, -1);
    parse_says(3, get_noun(current_id), -1, -1);
    parse_num(0, *CurrentMissionYear);
    MapWin->cOwner = current_id;
    const char* label = Factions[current_id].unk_102 ? "SWITCHING2" : "SWITCHING";
    popp(PopupScriptFile, label, 0, nullptr, nullptr);
    while (Factions[MapWin->cOwner].earned_techs_saved > 0) {
        --Factions[MapWin->cOwner].earned_techs_saved;
        tech_advance(MapWin->cOwner);
    }
}

/*
Thinker control_turn as a resumable loop. The original never returns while the game
runs: it calls Console_human_turn, the interactive map loop, for each human faction
and once at the end of a round without a human turn. Here those calls are the points
where this function returns, and ControlTurnMove records the human faction whose turn
is in progress (the same convention load_daemon uses for a loaded game). Calling it
again ends that human turn and continues with the next faction.
*/
int control_turn_run() {
    int current_id = 0;
    int skip_human_turn = 0;
    bool run_upkeep = true;
    s_control_turn_a = 0;

    if (*ControlTurnMove > 0 && *ControlTurnMove < MaxPlayerNum) {
        // Resume after the human turn of ControlTurnMove.
        current_id = *ControlTurnMove + 1;
        *ControlTurnMove = 0;
        skip_human_turn = 1;
        run_upkeep = false;
    } else {
        *ControlTurnMove = 0;
        if (!*CurrentTurn && is_human(MapWin->cOwner) && !*PbemActive && s_hooks.crash_landing) {
            s_hooks.crash_landing(MapWin->cOwner);
        }
        if (!*PbemActive) {
            draw_map(1);
        }
    }
    s_dword_93A940 = 1;
    while (1) {
        if (current_id >= MaxPlayerNum) {
            if (!skip_human_turn) {
                return TURN_ROUND_END; // Original: Console_human_turn(MapWin)
            }
            if (s_control_turn_a) {
                return TURN_STOPPED;
            }
            skip_human_turn = 0;
            current_id = 0;
            run_upkeep = true;
        }
        debug("control_turn %d %d %d\n", *CurrentTurn, current_id, MapWin->cOwner);
        if (run_upkeep) {
            *ControlTurnC = 1;
            turn_upkeep();
            if (end_of_game(1)) {
                return TURN_GAME_OVER;
            }
            run_upkeep = false;
        }
        if (is_alive(current_id)) {
            *CurrentFaction = current_id;
            if (is_human(current_id)
            && (current_id != MapWin->cOwner || (*GameMoreRules & MRULES_UNK_20) || *PbemActive)) {
                human_turn_switch(current_id);
            }
            if (s_control_turn_a) {
                return TURN_STOPPED;
            }
            *ControlTurnC = 1;
            faction_upkeep(current_id);
            if (end_of_game(0)) {
                return TURN_GAME_OVER;
            }
            *ControlTurnC = 0;
            *GameMoreRules &= ~MRULES_UNK_40;
            if (is_human(current_id)) {
                if (current_id == MapWin->cOwner || bit_count(FactionStatus[0]) > 1) {
                    *ControlTurnMove = current_id; // Original: Console_human_turn(MapWin)
                    return TURN_HUMAN;
                }
            } else {
                engine_notify(EV_MESSAGE, TE_AI_TURN, current_id, 0, 0, -1, -1, "ai_turn");
                mod_enemy_turn(current_id);
            }
            if (s_control_turn_a || end_of_game(0)) {
                return s_control_turn_a ? TURN_STOPPED : TURN_GAME_OVER;
            }
        }
        ++current_id;
    }
}

void __cdecl control_turn() {
    control_turn_run();
}

/*
Original: network multiplayer hands the turn to the next human through NetState.
Network play is not supported, so this only reports that there is no hand-over.
*/
int __cdecl next_player_turn() {
    if (!(*GameMoreRules & MRULES_UNK_10)) {
        return 0;
    }
    for (int fc_id = *CurrentFaction + 1; fc_id < MaxPlayerNum; ++fc_id) {
        if (is_alive(fc_id) && is_human(fc_id)) {
            return 0; // Original: message_data(0x4309, 0, fc_id, 0, 0, 0)
        }
    }
    s_dword_93A950 = 1;
    return 1;
}

/*
Thinker random_events. The global part (flares, dust, sunspots, betrayal reset,
base event countdown, perihelion) is ported; the choice of a base event is ported
too and the 23 individual base events are delegated to TurnHooks::random_base_event.
*/
static void random_events(int flag) {
    const uint32_t BEVENT_VISIBLE =
        (BEVENT_CLOUD_COVER|BEVENT_HEAT_WAVE|BEVENT_BUST|BEVENT_INDUSTRY|BEVENT_FAMINE|BEVENT_BUMPER);
    if (!flag) {
        if (*SolarFlaresEvent & 2) {
            *SolarFlaresEvent = 0;
        } else if (*SolarFlaresEvent & 1) {
            *SolarFlaresEvent = 2;
        }
        if (*DustCloudDuration < 0) {
            *DustCloudDuration = 0;
        }
        *SunspotDuration = max(*SunspotDuration, -1000) - 1;
        if (!*SunspotDuration) {
            wp1::pop2("NOMORESPOTS", "space_sm.pcx", -1);
        }
        for (int i = 1; i < MaxPlayerNum; i++) {
            for (int j = 1; j < MaxPlayerNum; j++) {
                if (i != j && is_human(i) && !is_human(j)
                && !has_treaty(j, i, DIPLO_WANT_REVENGE|DIPLO_VENDETTA)) {
                    set_treaty(j, i, DIPLO_UNK_800|DIPLO_SHALL_BETRAY, 0);
                }
            }
        }
        if (*DustCloudDuration > 0 && !--(*DustCloudDuration)) {
            wp1::pop2("NOMOREDUST", "stars_sm.pcx", -1);
        }
        for (int i = *BaseCount - 1; i >= 0; --i) {
            BASE* base = &Bases[i];
            if (base->event_flags & BEVENT_VISIBLE) {
                // Skip decrement if turns value is already zero
                if (base->random_event_turns > 0) {
                    base->random_event_turns--;
                }
                if (!base->random_event_turns) {
                    base->event_flags &= ~BEVENT_VISIBLE;
                }
            }
        }
        if (*GameState & STATE_PERIHELION_ACTIVE && !((game_year(*CurrentTurn) - 2180) % 80)) {
            *GameState &= ~STATE_PERIHELION_ACTIVE;
            wp1::pop2("PERIHELIONENDS", "stars_sm.pcx", -1);
            return;
        }
        if (!(conf.skip_random_events & 1) && *CurrentTurn > 50) {
            if (!((game_year(*CurrentTurn) - 2160) % 80)) {
                *GameState |= STATE_PERIHELION_ACTIVE;
                wp1::pop2("PERIHELION", "heat_sm.pcx", -1);
                return;
            }
        }
    }
    if (*GameRules & RULES_BELL_CURVE || *CurrentTurn < 75 - *DiffLevel * 10) {
        return;
    }
    if (!flag) {
        // This replaces game_reseed / game_random to keep the states separate
        map_rand.reseed(*MapRandomSeed + *CurrentTurn * 13);
    }
    const int base_id = map_rand.get(0, max(100, *BaseCount));
    if (base_id >= *BaseCount) {
        return;
    }
    BASE* base = &Bases[base_id];
    Faction* plr = &Factions[base->faction_id];
    if (base->pop_size <= 3 || plr->base_count <= 1 || base->event_flags & BEVENT_VISIBLE) {
        return;
    }
    const int event_value = map_rand.get(0, 22);
    if (conf.skip_random_events & (1 << (event_value + 1))) {
        return;
    }
    parse_num(0, conf.base_event_turns);
    parse_says(0, base->name, -1, -1); // Include initial base name by default
    if (s_hooks.random_base_event) {
        s_hooks.random_base_event(base_id, event_value);
    } else {
        not_implemented("random_events:base_event");
    }
}

void __cdecl do_fungal_towers() {
    auto spread_fungus = [](int x, int y) {
        bit_set(x, y, BIT_FUNGUS, 1);
        bit_set(x, y, BIT_FOREST|BIT_SOIL_ENRICHER|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_MAGTUBE|BIT_ROAD, 0);
        for (int i = 1; i < MaxPlayerNum; i++) {
            synch_bit(x, y, i);
        }
        draw_tile(x, y, 2);
    };
    for (int i = *VehCount - 1; i >= 0; --i) {
        VEH* veh = &Vehs[i];
        if (veh->unit_id == BSC_FUNGAL_TOWER) {
            veh->state |= VSTATE_HAS_MOVED;
            int x = veh->x;
            int y = veh->y;
            if (!(game_rand() % 5)) {
                int dir = game_rand() % 8;
                int x2 = wrap(x + TableOffsetX[dir + 1]);
                int y2 = y + TableOffsetY[dir + 1];
                if (on_map(x2, y2)) {
                    spread_fungus(x2, y2);
                }
            }
            spread_fungus(x, y);
            veh->morale = mod_morale_alien(i, 0);
            spot_all(i, 1);
        }
    }
}

void __cdecl turn_upkeep() {
    debug("turn_upkeep %d bases: %d vehs: %d\n", (*CurrentTurn)+1, *BaseCount, *VehCount);
    // Original: Thinker stamps its build date into the save here (ThinkerVars->build_date).
    if (*CurrentTurn == 0) {
        init_world_config();
    }
    // Original game turn upkeep starts here
    for (int veh_id = *VehCount - 1; veh_id >= 0; --veh_id) {
        VEH* veh = &Vehs[veh_id];
        if (veh->triad() == TRIAD_AIR && veh->range() && veh_speed(veh_id, 0) - veh->moves_spent > 0) {
            MAP* sq = mapsq(veh->x, veh->y);
            if (sq && sq->base_who() < 0 && !(sq->items & BIT_AIRBASE)
            && !stack_check(veh_id, 6, ABL_CARRIER, -1, -1)) {
                // Fix bug here that prevented the turn from advancing
                // while any needlejet in flight has moves left.
                if (veh->movement_turns < veh->range() - 1) {
                    ++veh->movement_turns;
                }
                veh->state |= VSTATE_HAS_MOVED;
                int dmg_val;
                if (veh->range() != 1 || veh->chassis_type() != CHS_COPTER) {
                    dmg_val = veh->cur_hitpoints();
                } else {
                    dmg_val = 3 * veh->reactor_type();
                }
                veh->damage_taken = clamp(veh->damage_taken + dmg_val, 0, 255);
                if (veh->max_hitpoints() - veh->damage_taken <= 0) {
                    kill(veh_id);
                }
            }
        }
    }
    *dword_90DB84 = 0; // action_terraform
    rankings(1);
    *CurrentMissionYear = game_year(++(*CurrentTurn));
    // Original: MapWin_main_caption(MapWin)
    engine_notify(EV_MESSAGE, TE_TURN_BEGIN, *CurrentTurn, *CurrentMissionYear, 0, -1, -1, "turn_begin");

    if (*ClimateFutureChange) {
        // Fix: 32-bit wrap-around arithmetic like the original x86 code, without
        // signed overflow on damaged values.
        int val = wrap_add(*ClimateValueA, *ClimateValueC);
        *ClimateValueC = val;
        if (*ClimateValueC >= *ClimateValueB) {
            *ClimateValueC = (int)((uint32_t)val - (uint32_t)*ClimateValueB);
            int alt_val = clamp(*ClimateFutureChange, -1, 1);
            *ClimateFutureChange -= alt_val;
            *MapSeaLevel += alt_val;
            if (s_hooks.world_climate) {
                s_hooks.world_climate();
            } else {
                not_implemented("world_climate");
            }
            draw_map(1);
            if (alt_val > 0) {
                popp(ScriptFile, "SEARISING", 0, "searis_sm.pcx", 0);
            } else {
                popp(ScriptFile, "SEAFALLING", 0, "seafal_sm.pcx", 0);
            }
        }
    }
    alien_fauna();
    do_fungal_towers();

    if ((*CurrentTurn == 4 && !game_randv(4))
    || (*CurrentTurn == 5 && !game_randv(3))
    || (*CurrentTurn == 6 && !game_randv(2))
    || *CurrentTurn >= 7) {
        bool aliens_arrive = false;
        for (int i = 1; i < MaxPlayerNum; i++) {
            Faction& plr = Factions[i];
            if (is_alien(i) || (!plr.base_count && !wp1::strcmpi(MFactions[i].filename, "FUNGBOY"))) {
                // TODO: investigate more consistent ways to mark factions for late spawns
                int flags = (int)(plr.player_flags | PFLAG_MAP_REVEALED);
                if (flags == -1 && plr.diff_level == -1 && !is_human(i) && !is_alive(i)) {
                    if (is_alien(i)) {
                        aliens_arrive = true;
                    }
                    set_alive(i, true);
                    mod_setup_player(i, -282, 0);
                }
            }
        }
        if (aliens_arrive) {
            popp(ScriptFile, "ALIENSARRIVE", 0, "al_land_sm.pcx", 0);
            if (s_hooks.interlude) {
                s_hooks.interlude(21, 0, 1, 0);
            }
        }
    }
    int caretake_id = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (is_alive(i) && !wp1::strcmpi(MFactions[i].filename, "CARETAKE")) { // Added is_alive check
            caretake_id = i;
        }
    }
    if (caretake_id) {
        for (int i = 0; i < *BaseCount; i++) {
            int faction_id = Bases[i].faction_id;
            if (faction_id != caretake_id
            && Bases[i].queue_items[0] == -FAC_ASCENT_TO_TRANSCENDENCE
            && !has_treaty(caretake_id, faction_id, DIPLO_VENDETTA)) {
                if (has_treaty(faction_id, caretake_id, DIPLO_PACT) && s_hooks.pact_ends) {
                    s_hooks.pact_ends(faction_id, caretake_id);
                }
                treaty_on(caretake_id, faction_id, DIPLO_VENDETTA|DIPLO_COMMLINK);
                set_treaty(caretake_id, faction_id, DIPLO_UNK_40, 1);
                Factions[faction_id].diplo_spoke[caretake_id] = *CurrentTurn;
                if (faction_id == *CurrentPlayerFaction) {
                    X_pops_11("NOTRANSCEND", 0x100000,
                        FactionPortraits ? FactionPortraits[caretake_id] : nullptr, pop_wait);
                }
            }
        }
    }
    for (int i = 1; i < MaxPlayerNum; i++) {
        for (int j = 1; j < MaxPlayerNum; j++) {
            if (i != j && is_alien(i) && is_alien(j)) {
                set_treaty(i, j, DIPLO_UNK_80000000|DIPLO_UNK_40000000|DIPLO_UNK_8000000|\
                    DIPLO_MAJOR_ATROCITY_VICTIM|DIPLO_ATROCITY_VICTIM|DIPLO_UNK_800|\
                    DIPLO_SHALL_BETRAY|DIPLO_WANT_REVENGE|DIPLO_VENDETTA|DIPLO_COMMLINK, 1);
                wp1::set_agenda(i, j, AGENDA_PERMANENT|AGENDA_UNK_800|AGENDA_UNK_400|AGENDA_UNK_20|\
                    AGENDA_FIGHT_TO_DEATH|AGENDA_UNK_4|AGENDA_UNK_1, 1);
                Factions[i].diplo_spoke[j] = *CurrentTurn;
            }
        }
    }
    random_events(0);
    if (*CurrentTurn == 75 && has_tech(Facility[FAC_BIOLOGY_LAB].preq_tech, *CurrentPlayerFaction)) {
        if (s_hooks.interlude) {
            s_hooks.interlude(1, 0, 1, 0);
        }
    }
    if (wp1::voice_of_planet()) {
        Faction& plr = Factions[*CurrentPlayerFaction];
        if (!(plr.player_flags & PFLAG_UNK_2000)) {
            plr.player_flags |= PFLAG_UNK_2000;
            parse_says(0, get_title(*CurrentPlayerFaction), -1, -1);
            parse_says(1, get_name(*CurrentPlayerFaction), -1, -1);
            popp(ScriptFile, "ASCENT", 0, "asctran_sm.pcx", 0);
        }
    }
    if ((*CurrentTurn & 3) == 1) {
        reset_territory();
    }
}

void __cdecl faction_upkeep(int faction_id) {
    Faction* f = &Factions[faction_id];
    debug("faction_upkeep %d %d\n", *CurrentTurn, faction_id);
    engine_notify(EV_MESSAGE, TE_FACTION_UPKEEP, faction_id, 0, 0, -1, -1, "faction_upkeep");

    wp1::init_save_game(faction_id);
    if (s_hooks.plans_upkeep) {
        s_hooks.plans_upkeep(faction_id);
    }
    // Original: reset_netmsg_status() (message window)
    s_control_upkeep_a = 1;
    social_upkeep(faction_id);
    repair_phase(faction_id);
    production_phase(faction_id);
    if (wp1::full_game_turn()) {
        allocate_energy(faction_id);
        enemy_diplomacy(faction_id);
        enemy_strategy(faction_id);
        /*
        Thinker-specific AI planning routines.
        Note that move_upkeep is only updated after all the production is done,
        so that the movement code can utilize up-to-date priority maps.
        This means we cannot use move_upkeep maps or variables in production phase.
        */
        mod_social_ai(faction_id, -1, -1, -1, -1, 0);
        if (s_hooks.probe_upkeep) {
            s_hooks.probe_upkeep(faction_id);
        }
        if (s_hooks.move_upkeep) {
            s_hooks.move_upkeep(faction_id, 0);
        }

        if (!is_human(faction_id) && *GameRules & RULES_VICTORY_ECONOMIC
        && has_tech(Rules->tech_preq_economic_victory, faction_id)) {
            int cost = corner_market(faction_id);
            if (!wp1::victory_done() && f->corner_market_cost <= 0 && f->energy_credits > cost) {
                f->corner_market_turn = *CurrentTurn + Rules->turns_corner_global_energy_market;
                f->corner_market_cost = cost;
                f->energy_credits -= cost;
                parse_says(0, get_title(faction_id), -1, -1);
                parse_says(1, get_name(faction_id), -1, -1);
                parse_says(2, get_noun(faction_id), -1, -1);
                parse_says(3, get_adjective(faction_id), -1, -1);
                ParseNumTable[0] = game_year(f->corner_market_turn);
                popp(ScriptFile, "CORNERWARNING", 0, "econwin_sm.pcx", 0);
            }
        }
    }
    for (int i = 0; i < *BaseCount; i++) {
        BASE* base = &Bases[i];
        if (base->faction_id == faction_id) {
            // Fix: clear hurry production flags moved to mod_base_upkeep
            base->state_flags &= ~BSTATE_UNK_1;
        }
    }
    f->energy_credits -= f->hurry_cost_total;
    f->hurry_cost_total = 0;
    if (f->energy_credits < 0) {
        f->energy_credits = 0;
    }
    if (!f->base_count && !has_active_veh(faction_id, PLAN_COLONY)) {
        mod_eliminate_player(faction_id, 0);
    }
    if (f->base_count && f->tech_research_id < 0 && s_net_upkeep_state != 1
    && !(*GameRules & RULES_SCN_NO_TECH_ADVANCES)) {
        f->tech_research_id = mod_tech_selection(faction_id);
    }
    s_control_upkeep_a = 0;
    // Original: Paths->xDst = -1; Paths->yDst = -1; (path cache, owned by WP3)

    if (wp1::full_game_turn()) {
        if (faction_id == MapWin->cOwner
        && !(*GameState & (STATE_COUNCIL_HAS_CONVENED | STATE_DISPLAYED_COUNCIL_AVAIL_MSG))
        && s_hooks.can_call_council && s_hooks.can_call_council(faction_id, 0)
        && !(*GameState & STATE_GAME_DONE)) {
            *GameState |= STATE_DISPLAYED_COUNCIL_AVAIL_MSG;
            popp(ScriptFile, "COUNCILOPEN", 0, "council_sm.pcx", 0);
        }
        if (!is_human(faction_id) && s_hooks.call_council) {
            s_hooks.call_council(faction_id);
        }
    }
    if (!*MultiplayerActive && *GamePreferences & PREF_BSC_AUTOSAVE_EACH_TURN
    && faction_id == MapWin->cOwner) {
        mod_auto_save();
    }
    flushlog();
}

void __cdecl repair_phase(int faction_id) {
    Faction* f = &Factions[faction_id];
    f->ODP_deployed = 0;
    Points tiles;

    for (int veh_id = 0; veh_id < *VehCount; veh_id++) {
        VEH* veh = &Vehs[veh_id];
        if (veh->faction_id != faction_id) {
            continue;
        }
        MAP* sq = mapsq(veh->x, veh->y);
        const bool at_base = sq && sq->is_base();
        const int triad = veh->triad();
        veh->iter_count = 0;
        veh->moves_spent = 0;
        veh->flags &= ~VFLAG_UNK_1000;
        veh->state &= ~(VSTATE_WORKING|VSTATE_UNK_2000|VSTATE_UNK_2);

        if (conf.activate_skipped_units) {
            veh->flags &= ~VFLAG_FULL_MOVE_SKIPPED;
        }
        if (sq && !at_base) {
            if (veh->faction_id == MapWin->cOwner || veh->is_visible(MapWin->cOwner)) {
                tiles.insert({veh->x, veh->y});
            }
        }
        if ( !((*CurrentTurn + veh_id) & 3) ) {
            veh->state &= ~VSTATE_UNK_800;
            if (veh->flags & VFLAG_UNK_2) {
                veh->flags &= ~VFLAG_UNK_2;
            } else {
                veh->flags &= ~VFLAG_UNK_1;
            }
        }
        if (veh->order == ORDER_SENTRY_BOARD || veh->order == ORDER_HOLD) {
            if (veh->waypoint_y[0]) {
                if (!--veh->waypoint_y[0]) {
                    veh->order = ORDER_NONE;
                }
            }
        }
        if (veh->state & VSTATE_UNK_8) {
            // Fix: sq checked for null (off-map vehicles in a damaged save)
            if (at_base
            || (sq && triad == TRIAD_LAND && sq->is_bunker())
            || (sq && triad == TRIAD_AIR && sq->is_airbase())) {
                veh->state &= ~VSTATE_UNK_8;
            }
        }
        if (veh->unit_id == BSC_FUNGAL_TOWER) {
            veh->faction_id = 0;
        }
        if (!sq || !veh->damage_taken
        || (veh->is_battle_ogre() && conf.repair_battle_ogre <= 0)
        || !(veh->unit_id == BSC_FUNGAL_TOWER || !(veh->state & VSTATE_HAS_MOVED))) {
            continue;
        }
        int value = conf.repair_minimal;
        int base_id = base_at(veh->x, veh->y);

        if (veh->unit_id != BSC_FUNGAL_TOWER) {
            if (veh->is_native_unit() && sq->is_fungus()) {
                value = conf.repair_fungus;
            }
            if (conf.repair_friendly > 0
            && whose_territory(faction_id, veh->x, veh->y, 0, 0) == faction_id) {
                value += conf.repair_friendly;
            }
            if (triad == TRIAD_AIR && sq->items & BIT_AIRBASE) {
                value += conf.repair_airbase;
            }
            if (triad == TRIAD_LAND && sq->items & BIT_BUNKER) {
                value += conf.repair_bunker;
            }
        }
        if (base_id >= 0 && !Bases[base_id].drone_riots_active()) {
            value += conf.repair_base;
            // Fix: consider secret projects built by the base owner instead of the veh owner
            if (!veh->is_native_unit()) {
                if ((triad == TRIAD_LAND && has_facility(FAC_COMMAND_CENTER, base_id))
                || (triad == TRIAD_SEA && has_facility(FAC_NAVAL_YARD, base_id))
                || (triad == TRIAD_AIR && has_facility(FAC_AEROSPACE_COMPLEX, base_id))) {
                    value += conf.repair_base_facility;
                }
            } else if (wp1::breed_mod(base_id, faction_id)) {
                value += conf.repair_base_native;
            }
        }
        bool repair_abl = false;
        if (triad == TRIAD_LAND) {
            if ((!veh_cargo(veh_id) && veh->order == ORDER_SENTRY_BOARD) || is_ocean(sq)) {
                int iter_id = veh_top(veh_id);
                while (iter_id >= 0) {
                    if (iter_id != veh_id && has_abil(Vehs[iter_id].unit_id, ABL_REPAIR)) {
                        repair_abl = true;
                    }
                    iter_id = Vehs[iter_id].next_veh_id_stack;
                }
                if (repair_abl) {
                    value *= 2;
                }
            }
        }
        if (has_project(FAC_NANO_FACTORY, faction_id)) {
            value += conf.repair_nano_factory;
        }
        if (veh->is_battle_ogre()) {
             value = min(value, conf.repair_battle_ogre);
        }
        int repair_limit = 0;
        int repair_value = value * veh->reactor_type();

        if (faction_id && !repair_abl && base_id < 0 && !has_project(FAC_NANO_FACTORY, faction_id)) {
            repair_limit = 2 * veh->reactor_type();
            if (sq->is_fungus()) {
                if (veh->is_native_unit()) {
                    repair_limit = 0;
                }
                if (has_project(FAC_XENOEMPATHY_DOME, faction_id)) {
                    repair_limit = 0;
                    repair_value += veh->reactor_type();
                }
            }
        }
        int damage = clamp(veh->damage_taken - repair_value, repair_limit, 255);
        if (damage < veh->damage_taken) {
            veh->damage_taken = damage;
            if (damage <= repair_limit && is_human(faction_id)
            && veh->order == ORDER_SENTRY_BOARD
            && (triad != TRIAD_LAND || !is_ocean(sq))) {
                veh->order = ORDER_NONE;
            }
        }
    }
    for (auto& p : tiles) {
        draw_tile(p.x, p.y, -1);
    }
}

void __cdecl production_phase(int faction_id) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    debug("production_phase %d %d\n", *CurrentTurn, faction_id);
    f->best_mineral_output = 0;
    f->energy_surplus_total = 0;
    f->facility_maint_total = 0;
    f->turn_commerce_income = 0;
    tech_effects(faction_id);

    for (int i = 1; i < MaxPlayerNum; i++) {
        if (faction_id != i && is_alive(faction_id) && is_alive(i)) {
            if (f->loan_balance[i] && !Factions[i].sanction_turns) {
                if (at_war(faction_id, i)) {
                    f->loan_balance[i] += f->loan_payment[i];
                } else {
                    int payment = clamp(f->energy_credits, 0, f->loan_payment[i]);
                    Factions[i].energy_credits += payment;
                    f->energy_credits -= payment;
                    f->loan_balance[i] -= payment;
                    if (payment < f->loan_payment[i]) {
                        f->loan_balance[i] += f->loan_payment[i] - payment;
                    }
                }
            }
        }
    }

    f->player_flags &= ~PFLAG_SELF_AWARE_COLONY_LOST_MAINT;
    s_energy_credits = f->energy_credits;
    /*
    Reset all fields listed below.
    int32_t social_support[8];
    int32_t social_psych[8][9];
    int32_t social_effic[9];
    int32_t unk_45;
    int32_t unk_46;
    int32_t unk_47;
    Fix: cleared field by field instead of one memset over the native struct layout.
    */
    memset(f->social_support, 0, sizeof(f->social_support));
    memset(f->social_psych, 0, sizeof(f->social_psych));
    memset(f->social_effic, 0, sizeof(f->social_effic));
    f->unk_45 = 0;
    f->unk_46 = 0;
    f->unk_47 = 0;

    f->nutrient_surplus_total = 0;
    f->labs_total = 0;

    for (int base_id = 0; base_id < *BaseCount; base_id++) {
        if (Bases[base_id].faction_id == faction_id) {
            if (mod_base_upkeep(base_id)) {
                base_id--; // Base was removed for some reason
            }
        }
    }
    /*
    Apply the original AI facility maintenance discounts
    These modifiers are not displayed in budget screens, they are just applied here
    */
    if (!is_human(faction_id) && *DiffLevel >= DIFF_THINKER) {
        if (f->facility_maint_total) {
            int value;
            if (*DiffLevel == DIFF_THINKER) {
                value = f->facility_maint_total / 3;
                f->energy_credits += value;
            } else {
                value = f->facility_maint_total * 2 / 3;
                f->energy_credits += value;
            }
            f->facility_maint_total -= value;
        }
    }
    /*
    INTEREST = Energy reserves interest.
    Non-zero = constant percentage per turn (including negative)
    Zero     = +1/base each turn
    */
    if (m->rule_flags & RFLAG_INTEREST) {
        int rule_interest = m->rule_interest;
        if (rule_interest) {
            f->energy_credits += f->energy_credits * rule_interest / 100;
        } else {
            for (int base_id = 0; base_id < *BaseCount; base_id++) {
                if (Bases[base_id].faction_id == faction_id) {
                    f->energy_credits++;
                    f->energy_surplus_total++;
                }
            }
        }
    }
    f->unk_18 = f->energy_surplus_total;
    f->unk_17 = f->energy_surplus_total + f->turn_commerce_income - f->facility_maint_total;

    if (*CurrentTurn == 1) {
        int techs = m->rule_tech_selected;
        s_skip_tech_screen_a = 1;
        while (--techs >= 0) {
            tech_advance(faction_id);
        }
        s_skip_tech_screen_a = 0;
    }
    if (wp1::full_game_turn()) {
        if (f->sanction_turns) {
            f->sanction_turns--;
            if (!f->sanction_turns && faction_id == MapWin->cOwner) {
                if (!is_alien(faction_id)) {
                    popp(ScriptFile, "SANCTIONSEND", 0, "council_sm.pcx", 0);
                } else {
                    popp(ScriptFile, "SANCTIONSENDALIEN", 0, "Alopdir.pcx", 0);
                }
            }
        }
    }
}

void __cdecl allocate_energy(int faction_id) {
    Faction* plr = &Factions[faction_id];
    MFaction* fac = &MFactions[faction_id];
    int alloc_val;
    int psych_val;

    if (is_human(faction_id)) {
        if (plr->diff_level > 1 && faction_id == MapWin->cOwner && plr->base_count > 1) {
            if (plr->energy_credits < 100 && 11 * plr->energy_credits - 10 * s_energy_credits < 0) {
                if (10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= 5) {
                    if (has_tech(Units[BSC_FORMERS].preq_tech, faction_id) && !(*CurrentTurn % 5)) {
                        popp(ScriptFile, "INFRASTRUCTURE2", 0, "genwarning_sm.pcx", 0);
                    }
                } else {
                    // Original: social_select(faction_id) opens the social engineering
                    // screen when the player accepts; the UI handles the answer.
                    popp(ScriptFile, "INFRASTRUCTURE", 0, "genwarning_sm.pcx", 0);
                }
            }
        }
        return;
    }
    alloc_val = 10 - plr->SE_alloc_psych - plr->SE_alloc_labs;
    if (plr->SE_police_pending >= -2) {
        if (plr->SE_police_pending == -2) {
            psych_val = 2;
        } else {
            psych_val = plr->SE_police_pending <= 1;
        }
    } else {
        psych_val = 4;
    }
    /*
    Modified psych energy allocation for rewritten governor specialist priorities
    where the scoring heuristic avoids situations in which the governor is forced
    to convert workers into specialists as counted by specialist_adjust.
    This replaces the original game version which used incorrect base iterators
    while the rest of this function uses previous decision methods.
    */
    if (has_project(FAC_TELEPATHIC_MATRIX, faction_id)) {
        plr->SE_alloc_psych = 0;
    } else {
        int pop_total = 0;
        int pop_score = 0;
        for (int base_id = 0; base_id < *BaseCount; base_id++) {
            BASE* b = &Bases[base_id];
            if (b->faction_id == faction_id && wp1::base_can_riot(base_id, true)) {
                int coeff_psych = 4;
                if (has_fac_built(FAC_HOLOGRAM_THEATRE, base_id)
                || (has_project(FAC_VIRTUAL_WORLD, faction_id)
                && has_fac_built(FAC_NETWORK_NODE, base_id))) {
                    coeff_psych += 2;
                }
                if (has_fac_built(FAC_RESEARCH_HOSPITAL, base_id)) {
                    coeff_psych += 1;
                }
                if (has_fac_built(FAC_NANOHOSPITAL, base_id)) {
                    coeff_psych += 1;
                }
                if (has_fac_built(FAC_TREE_FARM, base_id)) {
                    coeff_psych += 2;
                }
                if (has_fac_built(FAC_HYBRID_FOREST, base_id)) {
                    coeff_psych += 2;
                }
                pop_score += coeff_psych
                    * (b->drone_riots_active() ? 2 : 1)
                    * (b->assimilation_turns_left ? 1 : 2)
                    * (b->energy_surplus >= min(20, 2 + 2*b->pop_size) ? 2 : 1)
                    * max(0, (coeff_psych > 4 ? b->pop_size/4 : 0)
                    + 2*b->specialist_adjust + b->drone_total - b->talent_total);
                pop_total += b->pop_size;
            }
        }
        pop_score = pop_score * clamp(4 - plr->SE_police_pending, 1, 4) / 4;
        if (pop_score <= 3*pop_total) {
            if (plr->SE_alloc_psych > psych_val / 2) {
                if (pop_score <= 2*pop_total || plr->SE_alloc_psych - psych_val >= 2
                || plr->SE_police_pending >= (plr->SE_alloc_psych <= 1)) {
                    --plr->SE_alloc_psych;
                }
            }
        } else if (plr->SE_alloc_psych < psych_val) {
            if (++plr->SE_alloc_psych < psych_val && plr->SE_police_pending < 0) {
                plr->SE_alloc_psych += (pop_score >= 4*pop_total);
            }
        }
        plr->SE_alloc_psych = clamp(plr->SE_alloc_psych,
            (plr->SE_police_pending >= -1 || pop_score <= 2*pop_total ? 0 : 2), 4);
    }

    plr->SE_alloc_labs = (plr->AI_tech - plr->AI_power - plr->SE_alloc_psych + 11) / 2 - plr->AI_wealth;
    if (*CurrentTurn < 25 * (plr->AI_tech - plr->AI_power - plr->AI_wealth + 3)) {
        ++plr->SE_alloc_labs;
    }
    // Fix: guard the division (zero rule_techcost and turn in synthetic or modded rules)
    int labs_div = 2 * (*CurrentTurn + (plr->AI_power + plr->AI_wealth + 1) * fac->rule_techcost);
    if (labs_div > 0) {
        plr->SE_alloc_labs += plr->energy_credits / labs_div;
    }
    bool defense = false;
    for (int i = 1; i < MaxRegionNum; i++) {
        if (plr->region_total_bases[i]) {
            if (plr->region_base_plan[i] == PLAN_DEFENSE) {
                --plr->SE_alloc_labs;
                defense = true;
                break;
            }
            if (plr->region_base_plan[i] == PLAN_OFFENSE && (plr->AI_wealth || plr->AI_power)) {
                --plr->SE_alloc_labs;
            }
        }
    }
    if (plr->AI_tech && !plr->AI_wealth && !plr->AI_power && !defense) {
        ++plr->SE_alloc_labs;
    }
    if (alloc_val && 10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= alloc_val) {
        int sum = 0;
        for (int i = 0; i < *BaseCount; i++) {
            if (Bases[i].faction_id == faction_id) {
                sum += Bases[i].unk_total;
                for (int fc = 0; fc <= Fac_ID_Last; fc++) {
                    if (has_fac_built((FacilityId)fc, i)) {
                        sum -= wp1::fac_maint(fc, faction_id);
                    }
                }
            }
        }
        int base_mul = plr->base_count * (2 * plr->AI_wealth + 4 - plr->AI_tech) / 4;
        if (sum <= base_mul) {
            if (sum < base_mul / 2) {
                --plr->SE_alloc_labs;
            }
        } else {
            while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych >= alloc_val) {
                ++plr->SE_alloc_labs;
            }
        }
    }
    if (plr->tech_research_id == TECH_TranT) {
        --plr->SE_alloc_labs;
    }
    plr->SE_alloc_labs = max(0, min(plr->SE_alloc_labs,
        min(wp1::energy_limit(faction_id), 10 - plr->SE_alloc_psych)));
    while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych > wp1::energy_limit(faction_id)) {
        ++plr->SE_alloc_labs;
    }
    int effic_val = clamp(plr->SE_effic_2, 0, 99) + clamp(3 - plr->base_count, 0, 3) + 1;
    plr->SE_alloc_labs = clamp(plr->SE_alloc_labs, 0, 10);
    while (plr->SE_alloc_labs > 10 + effic_val - plr->SE_alloc_psych - plr->SE_alloc_labs) {
        --plr->SE_alloc_labs;
    }
    while (10 - plr->SE_alloc_labs - plr->SE_alloc_psych > effic_val + plr->SE_alloc_labs) {
        ++plr->SE_alloc_labs;
    }
}

/*
reset_territory exists only in terranx.exe (0x523DD0); this is new code built from
the rules the game documents (conceptsx.txt #CONCEPT31): a tile belongs to the
faction of the nearest base on the same continent (land) or in the same ocean (sea),
up to 7 tiles from a land base and 3 from a sea base. Thinker patches the two base
searches in this routine to base_find_3, which is used here with the same arguments
(any faction, the tile's region). Distances are vector_dist like base_find_3.
*/
void __cdecl reset_territory() {
    int owned = 0;
    for (int i = 1; i < MaxPlayerNum; i++) {
        memset(Factions[i].region_territory_tiles, 0, sizeof(Factions[i].region_territory_tiles));
    }
    for (int y = 0; y < *MapAreaY; y++) {
        for (int x = y & 1; x < *MapAreaX; x += 2) {
            MAP* sq = mapsq(x, y);
            if (!sq) {
                continue;
            }
            int owner = -1;
            int region = sq->region;
            int base_id = base_find_3(x, y, -1, region, -1, -1);
            if (base_id >= 0) {
                BASE* base = &Bases[base_id];
                int limit = is_ocean(base->x, base->y) ? 3 : 7;
                if (*BaseFindDist <= limit) {
                    owner = base->faction_id;
                }
            }
            sq->owner = (int8_t)owner;
            if (owner > 0) {
                owned++;
                if (Factions[owner].region_territory_tiles[region & 127] < 0xFFFF) {
                    Factions[owner].region_territory_tiles[region & 127]++;
                }
            }
        }
    }
    engine_notify(EV_MESSAGE, TE_TERRITORY, owned, 0, 0, -1, -1, "territory");
    draw_map(1);
}

/* ---- End Turn API ---- */

namespace {

/* Forwards everything to the previous sink and records it for engine_end_turn. */
class RecordingSink : public EventSink {
    public:
    EventSink* next = nullptr;
    std::vector<TurnEvent>* out = nullptr;

    TurnEvent make(int kind) {
        TurnEvent e;
        e.kind = kind;
        e.turn = *CurrentTurn;
        e.faction_id = *CurrentFaction;
        e.type = 0;
        e.a = e.b = e.c = e.d = 0;
        e.x = e.y = -1;
        return e;
    }
    int popup(const PopupRequest& request) override {
        int answer = next ? next->popup(request) : 0;
        TurnEvent e = make(TE_POPUP);
        e.a = answer;
        e.b = request.kind;
        e.text = request.label ? request.label : "";
        out->push_back(e);
        return answer;
    }
    void redraw(const RedrawRequest& request) override {
        if (next) {
            next->redraw(request);
        }
    }
    void notify(const EngineEvent& event) override {
        if (next) {
            next->notify(event);
        }
        if (event.type == EV_MESSAGE && event.text && event.a >= TE_TURN_BEGIN
        && event.a <= TE_TERRITORY && (!strcmp(event.text, "turn_begin")
        || !strcmp(event.text, "faction_upkeep") || !strcmp(event.text, "ai_turn")
        || !strcmp(event.text, "territory"))) {
            TurnEvent e = make(event.a);
            if (event.a == TE_TURN_BEGIN) {
                e.a = event.c; // year
            } else if (event.a == TE_TERRITORY) {
                e.a = event.b;
            } else {
                e.faction_id = event.b;
            }
            e.text = event.text;
            out->push_back(e);
            return;
        }
        TurnEvent e = make(TE_ENGINE);
        e.type = event.type;
        e.a = event.a;
        e.b = event.b;
        e.c = event.c;
        e.d = event.d;
        e.x = event.x;
        e.y = event.y;
        e.text = event.text ? event.text : "";
        out->push_back(e);
    }
};

} // namespace

TurnResult engine_end_turn() {
    TurnResult result;
    RecordingSink sink;
    sink.out = &result.events;
    sink.next = set_event_sink(&sink);
    if (*ControlTurnMove > 0 && *ControlTurnMove < MaxPlayerNum) {
        TurnEvent e = sink.make(TE_HUMAN_TURN_END);
        e.faction_id = *ControlTurnMove;
        result.events.push_back(e);
    }
    int status = control_turn_run();
    result.status = status;
    result.faction_id = -1;
    TurnEvent e = sink.make(TE_ROUND_END);
    if (status == TURN_HUMAN) {
        result.faction_id = *ControlTurnMove;
        e.kind = TE_HUMAN_TURN;
        e.faction_id = result.faction_id;
        result.events.push_back(e);
    } else if (status == TURN_ROUND_END) {
        result.events.push_back(e);
    } else if (status == TURN_GAME_OVER) {
        e.kind = TE_GAME_OVER;
        result.events.push_back(e);
    }
    set_event_sink(sink.next == headless_sink() ? nullptr : sink.next);
    result.turn = *CurrentTurn;
    result.year = *CurrentMissionYear;
    return result;
}

} // namespace engine
} // namespace alpha
