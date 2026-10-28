/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 research: technology availability, effects, selection, cost and
 * discovery side effects.
 * Ported from Thinker Mod src/tech.cpp and steal_tech from src/faction.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * tech_pick, tech_selection and tech_rate exist only in terranx.exe and are
 * written fresh here from the documented original rules (ENGINE-WP5.md).
 * Windows popups, sounds and network code are replaced by event-sink calls;
 * each such change is marked "Original:" and each behaviour change "Fix:".
 * tech_research belongs to WP1 and mod_tech_val/mod_tech_ai to WP6.
 */
#include "engine.h"
#include "tech.h"
#include "design.h"
#include "wp5_deps.h"

namespace alpha {
namespace engine {

/* Original interface globals (not saved; see tech.h). */
static int s_skip_tech_screen_a = 0;
int* SkipTechScreenA = &s_skip_tech_screen_a;
/* Original: dword_68F0F0 (first basic unit enabled by the tech),
 * dword_7591A4/dword_7591A8 (tech window state), dword_73396C. */
static int s_new_basic_unit = -1;
static char s_tech_buf[StrBufLen];

static bool revised_tech_cost() {
    // Not supported during multiplayer
    return conf.revised_tech_cost && !*MultiplayerActive;
}

/* Original: interlude(id, 0, 1, 0) plays a movie for the local player. */
static void wp5_interlude(int id) {
    engine_notify(EV_MESSAGE, id, 0, 0, 0, -1, -1, "interlude");
}

static const char* label_or(size_t index, const char* fallback) {
    const char* text = label_get(index);
    return (text && text[0]) ? text : fallback;
}

const char* tech_str(int tech_id) {
    if (tech_id >= 0 && tech_id < MaxTechnologyNum && Tech[tech_id].name != NULL) {
        return Tech[tech_id].name;
    }
    return "-";
}

/*
Craft an output string related to a specific technology. For techs outside the standard
range, craft a string related to world map, comm links or prototypes.
*/
void __cdecl say_tech(char* output, int tech_id, int incl_category) {
    size_t prev = strnlen(output, StrBufLen);
    char* buf = output + prev;
    size_t len = StrBufLen - prev;
    if (tech_id < -1) {
        snprintf(buf, len, "%s", label_or(TL_NotAvailable, "Not Available"));
    } else if (tech_id < 0) {
        snprintf(buf, len, "%s", label_or(TL_NONE, "NONE"));
    } else if (tech_id == 9999) {
        snprintf(buf, len, "%s", label_or(TL_WorldMap, "World Map"));
    } else if (tech_id < MaxTechnologyNum) {
        if (incl_category) {
            // Original: label_get(TL_TechTypeLetter + category) gives 'E', 'D', 'B', 'C'
            static const char letters[] = "EDBC";
            const char* letter = label_get(TL_TechTypeLetter + tech_category(tech_id));
            char fallback[2] = {letters[tech_category(tech_id) & 3], 0};
            snprintf(buf, len, "%s (%s%d)", tech_str(tech_id),
                (letter && letter[0]) ? letter : fallback, tech_level(tech_id, 0));
        } else {
            snprintf(buf, len, "%s", tech_str(tech_id));
        }
    } else if (tech_id < 97) {
        int faction_id = tech_id - MaxTechnologyNum;
        const char* comm = label_or(TL_CommFrequency, "Comm Frequency");
        if (*GameLanguage) {
            snprintf(buf, len, "%s (%s)", comm, get_noun(faction_id));
        } else {
            snprintf(buf, len, "%s %s", MFactions[faction_id].adj_name_faction, comm);
        }
    } else if (tech_id - 97 < MaxProtoNum) {
        snprintf(buf, len, "%s %s", Units[tech_id - 97].name, label_or(TL_Prototype, "Prototype"));
    }
}

/*
Calculate technology level for tech_id. Replaces function tech_recurse.
*/
int __cdecl tech_level(int tech_id, int lvl) {
    if (tech_id < 0 || tech_id > TECH_TranT || lvl > TECH_TranT) {
        return lvl;
    } else {
        int v1 = tech_level(Tech[tech_id].preq_tech1, lvl + 1);
        int v2 = tech_level(Tech[tech_id].preq_tech2, lvl + 1);
        return max(v1, v2);
    }
}

/*
Determine what category is the most important for tech_id.
If there is a tie, the order of precedence is as follows: growth > tech > wealth > power.
Return Value: Tech category id: growth (0), tech (1), wealth (2) or power (3).
*/
int __cdecl tech_category(int tech_id) {
    if (tech_id < 0 || tech_id > TECH_TranT) {
        return TCAT_GROWTH;
    }
    CTech& tech = Tech[tech_id];
    int label = TCAT_GROWTH;
    int value = tech.AI_growth;
    if (tech.AI_tech > value) {
        label = TCAT_TECH;
        value = tech.AI_tech;
    }
    if (tech.AI_wealth > value) {
        label = TCAT_WEALTH;
        value = tech.AI_wealth;
    }
    return (tech.AI_power > value) ? TCAT_POWER : label;
}

/*
Check to see whether provided faction can research a specific technology.
Includes checks to prevent SMACX specific techs from being researched in SMAC mode.
*/
int __cdecl tech_avail(int tech_id, int faction_id) {
    // Fix: reject out of range arguments instead of asserting
    if (faction_id < 0 || faction_id >= MaxPlayerNum || tech_id < 0) {
        return false;
    }
    if (has_tech(tech_id, faction_id) || tech_id >= MaxTechnologyNum || (!*ExpansionEnabled
    && (tech_id == TECH_PrPsych || tech_id == TECH_FldMod || tech_id == TECH_AdapDoc
    || tech_id == TECH_AdapEco || tech_id == TECH_Bioadap || tech_id == TECH_SentRes
    || tech_id == TECH_SecMani || tech_id == TECH_NewMiss || tech_id == TECH_BFG9000))) {
        return false;
    }
    int preq_tech_1 = Tech[tech_id].preq_tech1;
    int preq_tech_2 = Tech[tech_id].preq_tech2;
    if (preq_tech_1 <= TECH_Disable || preq_tech_2 <= TECH_Disable) {
        return false;
    }
    return (has_tech(preq_tech_1, faction_id) && has_tech(preq_tech_2, faction_id));
}

/*
Calculate the tech related bonuses for commerce and resource production in fungus.
*/
void __cdecl tech_effects(int faction_id) {
    Faction* f = &Factions[faction_id];
    MFaction* m = &MFactions[faction_id];
    f->tech_commerce_bonus = m->rule_commerce;
    f->tech_fungus_nutrient = 0;
    f->tech_fungus_mineral = 0;
    f->tech_fungus_energy = 0;
    f->tech_fungus_unused = 0;

    for (int tech_id = 0; tech_id < MaxTechnologyNum; tech_id++) {
        if (has_tech(tech_id, faction_id)) {
            uint32_t flags = Tech[tech_id].flags;
            if (flags & TFLAG_INC_NUTRIENT_FUNGUS) {
                f->tech_fungus_nutrient++;
            }
            if (flags & TFLAG_INC_MINERALS_FUNGUS) {
                f->tech_fungus_mineral++;
            }
            if (flags & TFLAG_INC_ENERGY_FUNGUS) {
                f->tech_fungus_energy++;
            }
            if (flags & TFLAG_INC_COMMERCE) {
                f->tech_commerce_bonus++;
            }
        }
    }
    for (int i = 0; i < m->faction_bonus_count && i < 8; i++) {
        if (m->faction_bonus_id[i] == RULE_FUNGNUTRIENT) {
            f->tech_fungus_nutrient += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == RULE_FUNGMINERALS) {
            f->tech_fungus_mineral += m->faction_bonus_val1[i];
        } else if (m->faction_bonus_id[i] == RULE_FUNGENERGY) {
            f->tech_fungus_energy += m->faction_bonus_val1[i];
        }
    }
    f->tech_fungus_nutrient = max(0, f->tech_fungus_nutrient);
    f->tech_fungus_mineral = max(0, f->tech_fungus_mineral);
    f->tech_fungus_energy = max(0, f->tech_fungus_energy);

    if (f->SE_economy_pending > 2) {
        f->tech_commerce_bonus++;
        if (f->SE_economy_pending > 3) {
            f->tech_commerce_bonus++;
            if (f->SE_economy_pending > 4) {
                f->tech_commerce_bonus++;
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * tech_pick / tech_selection (terranx.exe, written fresh)
 * ------------------------------------------------------------------------- */

static TechChooser s_tech_chooser = nullptr;
static void* s_tech_chooser_ctx = nullptr;

TechChooser set_tech_chooser(TechChooser chooser, void* context) {
    TechChooser prev = s_tech_chooser;
    s_tech_chooser = chooser;
    s_tech_chooser_ctx = context;
    return prev;
}

int research_pool(int faction_id, int other_faction_id, TechChoice* out) {
    TechChoice local;
    TechChoice& c = out ? *out : local;
    c.faction_id = faction_id;
    c.other_faction_id = other_faction_id;
    c.flag = 0;
    c.label = nullptr;
    c.count = 0;
    if (!valid_player(faction_id) && faction_id != 0) {
        return 0;
    }
    if (other_faction_id >= 0) {
        // Trade/steal list: every tech the other faction owns that this one can research.
        for (int id = 0; id < MaxTechnologyNum; id++) {
            if (tech_avail(id, faction_id) && has_tech(id, other_faction_id)) {
                c.ids[c.count] = id;
                c.categories[c.count] = tech_category(id);
                c.count++;
            }
        }
        return c.count;
    }
    // Normal directed research pool (see tech.h and RESEARCH-POOL rules).
    const int ranking = Factions[faction_id].tech_ranking;
    for (int group = 0; group < 4; group++) {
        for (int id = 0; id < MaxTechnologyNum; id++) {
            if (!tech_avail(id, faction_id)) {
                continue;
            }
            if (c.count) {
                int64_t n = (int64_t)id - ranking + faction_id;
                if (n % 3 == 0 && (id >= 7 || ranking != 0)) {
                    continue;
                }
            }
            if (tech_category(id) != group) {
                continue;
            }
            c.ids[c.count] = id;
            c.categories[c.count] = group;
            c.count++;
        }
    }
    return c.count;
}

int tech_tradeable(int faction_id, int other_faction_id, int* ids, int max_ids) {
    int count = 0;
    if (other_faction_id < 0 || other_faction_id >= MaxPlayerNum) {
        return 0;
    }
    for (int id = 0; id < MaxTechnologyNum; id++) {
        if (tech_avail(id, faction_id) && has_tech(id, other_faction_id)) {
            if (ids && count < max_ids) {
                ids[count] = id;
            }
            count++;
        }
    }
    return count;
}

static int ask_tech_choice(const TechChoice& c) {
    if (c.count <= 0) {
        return -1;
    }
    int choice;
    if (s_tech_chooser) {
        choice = s_tech_chooser(c, s_tech_chooser_ctx);
        for (int i = 0; i < c.count; i++) {
            if (c.ids[i] == choice) {
                return choice;
            }
        }
        return -1;
    }
    PopupRequest req = {};
    req.kind = POP_X_POP;
    req.filename = ScriptFile;
    req.label = c.label ? c.label : "TECHPICK";
    req.image = nullptr;
    req.sprite = nullptr;
    req.flags = c.flag;
    req.sound_id = -1;
    req.list_index = 0;
    req.faction_id = c.other_faction_id;
    for (int i = 0; i < 10; i++) {
        req.strings[i] = (i < c.count) ? tech_str(c.ids[i]) : "";
        req.numbers[i] = (i < c.count) ? c.ids[i] : -1;
    }
    int index = event_sink()->popup(req);
    return (index >= 0 && index < c.count) ? c.ids[index] : -1;
}

/*
Fresh implementation of the original tech_pick.
- other_faction_id >= 0 (probe steal, TECHSTEAL ability, trades): choose one of
  the other faction's techs this faction can research. A human local player
  chooses (label ACQUIRETECH/STEALTECH); computer players take the highest
  tech_val, the first in id order on a tie. Returns -1 when there is nothing.
- normal research: computer players, blind research and turn 0 use tech_ai;
  a human chooses from the directed pool.
*/
int __cdecl tech_pick(int faction_id, int flag, int other_faction_id, const char* label) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum) {
        return -1;
    }
    TechChoice c;
    research_pool(faction_id, other_faction_id, &c);
    c.flag = flag;
    c.label = label;
    bool local_human = is_human(faction_id)
        && (faction_id == MapWin->cOwner || !*MultiplayerActive);
    if (other_faction_id >= 0) {
        if (!c.count) {
            return -1;
        }
        if (local_human) {
            return ask_tech_choice(c);
        }
        int best = c.ids[0];
        int best_val = INT_MIN;
        for (int i = 0; i < c.count; i++) {
            int val = mod_tech_val(c.ids[i], faction_id, 1);
            if (val > best_val) {
                best_val = val;
                best = c.ids[i];
            }
        }
        return best;
    }
    bool automatic = !is_human(faction_id) || (*GameRules & RULES_BLIND_RESEARCH)
        || !*CurrentTurn || !local_human;
    if (automatic) {
        int tech_id = mod_tech_ai(faction_id);
        if (tech_id >= 0 && tech_avail(tech_id, faction_id)) {
            return tech_id;
        }
        // Fix: never return a tech the faction cannot research (tech_ai placeholder
        // or empty choice): use the first entry of the normal pool instead.
        return c.count ? c.ids[0] : -1;
    }
    return ask_tech_choice(c);
}

int __cdecl tech_selection(int faction_id) {
    return mod_tech_selection(faction_id);
}

int __cdecl mod_tech_selection(int faction_id) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum) {
        return -1;
    }
    Faction& plr = Factions[faction_id];
    // Original: network play waits for the remote human's choice (PICKINGTECH)
    // and synchronises it; multiplayer is not supported by this engine.
    int tech_id = mod_tech_pick(faction_id, 0, -1, 0);
    plr.tech_research_id = tech_id;
    return tech_id;
}

/*
Normally the game engine would recalculate research cost in tech_research
before the next tech is selected in tech_selection > tech_pick.
This version recalculates tech cost immediately after a new tech is selected.
Note that tech_pick is also used for TECHSTEAL faction ability.
*/
int __cdecl mod_tech_pick(int faction_id, int flag, int other_faction_id, const char* label) {
    Faction& plr = Factions[faction_id];
    int tech_id = tech_pick(faction_id, flag, other_faction_id, label);
    debug("tech_pick %d %d %d prev: %d %d tech: %d %s\n", *CurrentTurn, faction_id, other_faction_id,
        plr.tech_research_id, plr.tech_cost, tech_id, tech_str(tech_id));
    if (other_faction_id < 0 && revised_tech_cost()) {
        if (tech_id >= 0 && (tech_id != plr.tech_research_id || plr.tech_cost <= 0)) {
            plr.tech_cost = tech_alt_cost(tech_id, faction_id);
        }
    }
    flushlog();
    return tech_id;
}

bool research_change_needs_confirmation(int faction_id) {
    Faction& plr = Factions[faction_id];
    return plr.tech_research_id >= 0 && plr.tech_accumulated != 0;
}

/*
Fresh: the post-chooser field transition of the original research screen
(Console_set_tech), matching the manual-research-choice evidence:
declined -> nothing; same target -> nothing; no previous target -> set;
changed target -> set and halve labs (truncating toward zero). The cached cost
is kept like the original; the revised cost mode recalculates it like mod_tech_pick.
*/
int research_change(int faction_id, int tech_id, bool confirmed) {
    if (faction_id <= 0 || faction_id >= MaxPlayerNum || !tech_avail(tech_id, faction_id)) {
        return RESEARCH_INVALID;
    }
    Faction& plr = Factions[faction_id];
    if (tech_id == plr.tech_research_id) {
        return RESEARCH_SAME;
    }
    if (research_change_needs_confirmation(faction_id) && !confirmed) {
        return RESEARCH_NEEDS_CONFIRMATION;
    }
    int result = RESEARCH_SET;
    if (plr.tech_research_id >= 0) {
        plr.tech_accumulated /= 2;
        result = RESEARCH_CHANGED;
    }
    plr.tech_research_id = tech_id;
    if (revised_tech_cost()) {
        plr.tech_cost = tech_alt_cost(tech_id, faction_id);
    }
    return result;
}

/*
Original game research cost (terranx.exe tech_rate, also described by
OpenSMACX technology.cpp::tech_rate). The cached value tech_cost is returned
while it is nonnegative; tech_research sets it to -1 to force a recalculation.
*/
int __cdecl tech_rate(int faction_id) {
    Faction& plr = Factions[faction_id];
    if (plr.tech_cost >= 0) {
        return plr.tech_cost;
    }
    if (!Rules->tech_discovery_rate) {
        return 999999999;
    }
    int plr_tech_rating = clamp(plr.tech_ranking + 2 * plr.earned_techs_saved - plr.unk_27, 2, 9999);
    int max_tech = 0;
    for (int i = 0; i < MaxPlayerNum; ++i) {
        max_tech = max(max_tech, Factions[i].tech_ranking + 2 * Factions[i].earned_techs_saved);
    }
    bool is_plr = is_human(faction_id);
    bool is_stagnate = *GameRules & RULES_TECH_STAGNATION;
    int half_max_tech = max_tech / 2;
    int half_plr_tech = plr_tech_rating / 2;
    int diff = is_plr ? plr.diff_level : *DiffLevel;
    int diff_adj = (diff < 3) ? (diff + 1) : diff;
    int diff_val = is_plr ? 3 : *DiffLevel;
    int factor = is_plr ? (4 * diff_adj + 8) : (29 - 3 * diff_adj);
    factor = clamp(factor, 12 - half_plr_tech, 12 + half_plr_tech);
    int turn_penalty = half_plr_tech - (*CurrentTurn / (is_stagnate ? 12 : 8));
    turn_penalty = clamp(turn_penalty, 0, (factor * (is_stagnate ? 3 : 2)) / 2);
    int factor_adj = turn_penalty + factor;
    // Fix: avoid a division by zero when an AI difficulty of 8 is configured
    int reduction = (diff_val == 8) ? 0 : (half_max_tech - diff_val - half_plr_tech + 7) / (8 - diff_val);
    reduction = clamp(reduction, 0, (diff_val * factor_adj / 10) + 1);
    int tech_multiplier = clamp(half_plr_tech - clamp(plr.SE_research_base, -1, 1), 1, 99999);
    // Fix: avoid possible overflow issues with custom rules by casting values to int64_t
    int64_t cost = int64_t(factor_adj - reduction) * tech_multiplier;
    if (Rules->tech_discovery_rate != 100) {
        cost = (cost * 100) / Rules->tech_discovery_rate;
    }
    if (MFactions[faction_id].rule_techcost != 100) {
        cost = (cost * MFactions[faction_id].rule_techcost) / 100;
    }
    cost = cost * *MapAreaSqRoot / 56;
    if (is_stagnate) {
        cost += cost / 2;
    }
    cost = clamp(cost, int64_t(1), int64_t(99999999));
    return (int)cost;
}

int __cdecl mod_tech_rate(int faction_id) {
    Faction& plr = Factions[faction_id];
    if (revised_tech_cost()) {
        return (plr.tech_cost > 0 ? plr.tech_cost : 9999);
    }
    // Calculate the original game tech cost values (Thinker body; see tech_rate)
    return tech_rate(faction_id);
}

/*
Determine if preq_tech_id is a prerequisite of parent_tech_id within descending range.
Return Value: Is preq_tech_id prerequisite of parent_tech_id? true/false
*/
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range) {
    if (preq_tech_id < 0 || parent_tech_id < 0 || parent_tech_id >= MaxTechnologyNum) {
        return false;
    }
    if (preq_tech_id == parent_tech_id) {
        return true;
    }
    if (range <= 0) {
        return false;
    }
    return tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech1, range - 1)
        || tech_is_preq(preq_tech_id, Tech[parent_tech_id].preq_tech2, range - 1);
}

int __cdecl tech_advance(int faction_id) {
    Faction* plr = &Factions[faction_id];
    // Original: log_say("Earning a Tech", ...); network and PBEM deferral
    // (earned_techs_saved) only apply to multiplayer games.
    if (*PbemActive && is_human(faction_id) && faction_id != MapWin->cOwner) {
        ++plr->earned_techs_saved;
        return -1;
    }
    int tech_id = plr->tech_research_id;
    if (tech_id >= 0 || (tech_id = mod_tech_selection(faction_id), tech_id >= 0)) {
        plr->tech_research_id = -1;
        tech_achieved(faction_id, tech_id, 0, 0);
        if (!is_human(faction_id)) {
            int base_id = *CurrentBaseID;
            mod_bases_reset(-1, faction_id, 0);
            if (*CurrentTurn) {
                if (base_id >= 0 && base_id < *BaseCount) {
                    set_base(base_id);
                    base_compute(0);
                }
            }
        }
        if ((*MultiplayerActive || faction_id == MapWin->cOwner)
        && !*SkipTechScreenA && !*SkipTechScreenB && plr->tech_research_id < 0) {
            plr->tech_research_id = mod_tech_selection(faction_id);
        }
    }
    return tech_id;
}

/*
Applies all side effects of gaining a tech: notifications, prototype designs,
free facility grants, map reveal, social engineering prompts, and propagation
to other factions via Planetary Datalinks and TECHSHARE faction ability.
Also multiplexes several unrelated events through tech_id's range.

faction_id   - Faction receiving the item. Must be in range 0..7.
tech_id      - 0..88    : a real technology (TECH_TranT == 88 is the last one).
               89..96   : grant comm-frequency with faction (tech_id - 89).
               97..9998 : propose prototype for unit_id (tech_id - 97).
               >= 9999  : gift/trade world map from faction_id_2.
faction_id_2 - Meaningful only when tech_id < 89 or >= 9999:
               >0 : source faction_id, trade/gift/steal.
                0 : no other source, researched normally.
               -1 : internal, granted via Planetary Datalinks propagation.
               -2 : internal, granted via TECHSHARE propagation.
is_share     - Non-zero for a shared/leaked tech (changes popup text).
*/
void __cdecl tech_achieved(int faction_id, int tech_id, int faction_id_2, int is_share) {
    if (!(faction_id >= 0 && faction_id < MaxPlayerNum)) {
        return;
    }
    if (tech_id < 0) {
        return;
    }
    Faction* plr = &Factions[faction_id];
    MFaction* fac = &MFactions[faction_id];
    const int bm_fac = 1 << faction_id;

    // Original: log_say("Gaining a TECH", ...)
    debug("tech_achieved %d %d %d %d\n", *CurrentTurn, faction_id, tech_id, faction_id_2);
    if (is_human(faction_id)) {
        // Original: ambience(201)
        engine_notify(EV_MESSAGE, 201, faction_id, 0, 0, -1, -1, "ambience");
    }
    if (tech_id >= 9999) {
        if (!(faction_id_2 >= 0 && faction_id_2 < MaxPlayerNum)) {
            return;
        }
        const int bm_tgt = 1 << faction_id_2;
        plr->traded_maps |= bm_tgt | Factions[faction_id_2].traded_maps;
        for (int i = 0; i < *MapAreaTiles; ++i) {
            MAP* sq = &(*MapTiles)[i];
            if (sq->visibility & bm_tgt) {
                sq->visibility |= bm_fac;
                if (faction_id && faction_id_2) {
                    sq->visible_items[faction_id - 1] |= sq->items & sq->visible_items[faction_id_2 - 1];
                }
            }
        }
        for (int i = 0; i < *BaseCount; ++i) {
            if ((Bases[i].visibility & bm_tgt) || Bases[i].faction_id == faction_id_2) {
                spot_base(i, faction_id);
            }
        }
        if (faction_id == MapWin->cOwner) {
            draw_map(1);
            *GameDrawState |= 4u;
        }
        return;
    }
    if (tech_id >= 89) {
        if (tech_id < 97) {
            treaty_on(faction_id, tech_id - 89, DIPLO_COMMLINK);
            return;
        }
        if (tech_id - 97 >= MaxProtoNum) {
            return;
        }
        UNIT* src = &Units[tech_id - 97];
        int uid = mod_propose_proto(
            faction_id,
            (VehChassis)src->chassis_id,
            (VehWeapon)src->weapon_id,
            (VehArmor)src->armor_id,
            (VehAblFlag)src->ability_flags,
            (VehReactor)src->reactor_id,
            PLAN_PROTOTYPE_DONE,
            src->name);
        if (uid >= 0) {
            if (*MultiplayerActive && is_human(faction_id)) {
                Units[uid].unit_flags |= UNIT_UNK_100;
            } else {
                prune_protos(faction_id, uid, 0);
                upgrade_any_prototypes(faction_id);
            }
        }
        return;
    }
    if (plr->tech_research_id == tech_id) {
        plr->tech_research_id = -1;
    }
    plr->tech_trade_source[tech_id] = faction_id_2 <= 0 ? 0 : faction_id_2;
    int is_first = TechOwners[tech_id] == 0;
    TechOwners[tech_id] |= (1 << faction_id);
    ++plr->tech_achieved;
    if (!*CurrentTurn || *SkipTechScreenA) {
        return;
    }
    ++plr->tech_ranking;
    if (plr->unk_27) {
        --plr->unk_27;
    }
    ++plr->tech_ranking;
    if (plr->unk_27) {
        --plr->unk_27;
    }
    if (tech_id == TECH_TranT) {
        ++plr->tech_count_transcendent;
    }
    int best_rc_id = 0;
    for (int rc_id = REC_FUSION; rc_id <= REC_SINGULARITY; ++rc_id) {
        if (tech_id == Reactor[rc_id - 1].preq_tech) {
            for (int i = 0; i < MaxProtoFactionNum; ++i) {
                int unit_id = i + (faction_id * MaxProtoFactionNum);
                // The original game does not check for MPREF_BSC_AUTO_PRUNE_OBS_VEH when discovering
                // better reactors, usually resulting in dozens of popups for each reactor tech
                UNIT* cur = &Units[unit_id];
                if ((cur->unit_flags & UNIT_ACTIVE)
                && !(cur->obsolete_factions & bm_fac) && cur->reactor_id < rc_id
                && (!conf.ignore_reactor_power || !is_human(faction_id)
                || *GameMorePreferences & MPREF_BSC_AUTO_PRUNE_OBS_VEH)) {
                    // Fix: remove redundant loop code
                    int uid = mod_propose_proto(
                        faction_id,
                        (VehChassis)cur->chassis_id,
                        (VehWeapon)cur->weapon_id,
                        (VehArmor)cur->armor_id,
                        (VehAblFlag)cur->ability_flags,
                        (VehReactor)rc_id,
                        (cur->unit_flags & UNIT_PROTOTYPED) ? PLAN_PROTOTYPE_DONE : PLAN_AUTO_CALCULATE,
                        0);
                    if (uid >= 0 && uid != unit_id) { // Fix: skip a failed proposal (-1)
                        if (*MultiplayerActive && is_human(faction_id)) {
                            Units[uid].unit_flags |= UNIT_UNK_100;
                        } else {
                            prune_protos(faction_id, uid, 0);
                            upgrade_any_prototypes(faction_id);
                        }
                    }
                    best_rc_id = rc_id;
                }
            }
        }
    }
    if (best_rc_id && faction_id == MapWin->cOwner && !*MultiplayerActive) {
        parse_says(0, Reactor[best_rc_id - 1].name ? Reactor[best_rc_id - 1].name : "", -1, -1);
        X_pop("REACTORUPGRADE", 0);
    }
    s_new_basic_unit = -1;
    if (*MultiplayerActive) {
        plr->player_flags |= PFLAG_MULTI_TECH_ACHIEVED;
    } else if (!is_human(faction_id) || *GamePreferences & PREF_BSC_AUTO_DESIGN_VEH) {
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            Units[(faction_id * MaxProtoFactionNum) + i].unit_flags &= ~0xC0u;
        }
        consider_designs(faction_id);
        if (faction_id == MapWin->cOwner) {
            for (int i = 0; i < MaxProtoFactionNum; ++i) {
                UNIT* unit = &Units[i];
                if ((unit->unit_flags & UNIT_ACTIVE) && tech_id == unit->preq_tech && s_new_basic_unit < 0) {
                    s_new_basic_unit = i;
                }
            }
        }
    }
    if (faction_id == MapWin->cOwner || *GameState & STATE_OMNISCIENT_VIEW) {
        s_tech_buf[0] = 0;
        say_tech(s_tech_buf, tech_id, 0);
        parse_says(0, s_tech_buf, -1, -1);
        parse_says(1, get_noun(faction_id), -1, -1);
        if (faction_id_2 <= 0) {
            if (faction_id_2 < 0) {
                if (faction_id_2 == -1) {
                    parse_says(2, Facility[FAC_PLANETARY_DATALINKS].name
                        ? Facility[FAC_PLANETARY_DATALINKS].name : "", -1, -1);
                } else {
                    parse_says(2, label_or(TL_DataLinks, "Data Links"), -1, -1);
                }
            }
        } else {
            parse_says(2, get_noun(faction_id_2), -1, -1);
        }
        s_tech_buf[0] = 0;
        if (faction_id == MapWin->cOwner) {
            strcat(s_tech_buf, "WE");
        } else {
            strcat(s_tech_buf, "THEY");
        }
        if (faction_id_2) {
            strcat(s_tech_buf, "ACQUIRE");
        } else {
            strcat(s_tech_buf, "DEVELOP");
        }
        if (is_share) {
            s_tech_buf[0] = 0;
            strcat(s_tech_buf, "WEGETSHARETECH");
        }
        // Original: a Popup with the TECH<n> blurb (BlurbsxFile), the tech icon,
        // FX_play(51) and NewTechWin_exec for the local player. The UI shows the
        // new-tech window from this popup (numbers[0] = tech_id is set below).
        parse_num(0, tech_id);
        parse_num(1, faction_id_2);
        X_pop_2(PopupScriptFile, s_tech_buf, 0);
    }
    int has_freefac = 0;
    for (int i = 0; i < fac->faction_bonus_count && i < 8; ++i) {
        if (fac->faction_bonus_id[i] == RULE_FREEFAC
        && fac->faction_bonus_val1[i] >= 0 && fac->faction_bonus_val1[i] <= SP_ID_Last
        && tech_id == Facility[fac->faction_bonus_val1[i]].preq_tech) {
            has_freefac = 1;
            for (int base_id = 0; base_id < *BaseCount; ++base_id) {
                if (Bases[base_id].faction_id == faction_id) {
                    set_fac((FacilityId)fac->faction_bonus_val1[i], base_id, 1);
                }
            }
            if (faction_id == MapWin->cOwner) {
                parse_says(0, tech_str(tech_id), -1, -1);
                parse_says(1, Facility[fac->faction_bonus_val1[i]].name
                    ? Facility[fac->faction_bonus_val1[i]].name : "", -1, -1);
                NetMsg_pop(NetMsg, "FREEFACTECH", -5000, 0, 0);
            }
        }
    }
    (void)has_freefac; // Original: NetDaemon_synch after granting free facilities
    for (int i = 0; i < fac->faction_bonus_count && i < 8; ++i) {
        if (fac->faction_bonus_id[i] == RULE_FREEABIL
        && fac->faction_bonus_val1[i] >= 0 && fac->faction_bonus_val1[i] < MaxAbilityNum
        && tech_id == Ability[fac->faction_bonus_val1[i]].preq_tech
        && faction_id == MapWin->cOwner) {
            parse_says(0, tech_str(tech_id), -1, -1);
            parse_says(1, Ability[fac->faction_bonus_val1[i]].name
                ? Ability[fac->faction_bonus_val1[i]].name : "", -1, -1);
            NetMsg_pop(NetMsg, "FREEABILTECH", -5000, 0, 0);
        }
    }
    if (Tech[tech_id].flags & TFLAG_SECRETS) {
        mon_secrets_of_tech(faction_id);
    }
    if (faction_id == MapWin->cOwner
    && tech_id == Terraform[FORMER_FOREST].preq_tech && !*MultiplayerActive) {
        parse_says(0, tech_str(tech_id), -1, -1);
        X_pop_2("TUTOR", "TREETIME", 0);
    }
    if (faction_id == MapWin->cOwner
    && (*GamePreferences & PREF_BSC_AUTO_DESIGN_VEH)
    && s_new_basic_unit >= 0 && !*MultiplayerActive) {
        int choice = 0;
        *dword_90EA3C = 0;
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            // Fix: remove redundant Spore Launcher reference
            if ((Units[i].unit_flags & UNIT_ACTIVE)
            && tech_id == Units[i].preq_tech && !is_native_unit(i)) {
                if (choice) {
                    if (choice > 0) {
                        design_new_veh(-faction_id, i);
                    }
                } else {
                    if (!X_pop("ASKSEEDESIGN", 0)) {
                        choice = -1;
                    } else {
                        choice = 1;
                        design_new_veh(-faction_id, i);
                    }
                }
            }
        }
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            int uid = i + (faction_id * MaxProtoFactionNum);
            UNIT* cur = &Units[uid];
            if (cur->unit_flags & UNIT_UNK_80 && !(cur->obsolete_factions & bm_fac)) {
                if (choice) {
                    if (choice > 0) {
                        design_new_veh(-faction_id, uid);
                    }
                } else {
                    if (!X_pop("ASKSEEDESIGN", 0)) {
                        choice = -1;
                    } else {
                        choice = 1;
                        design_new_veh(-faction_id, uid);
                    }
                }
                cur->unit_flags &= ~UNIT_UNK_80;
            }
        }
        // Original: DesignWin_shutdown(DesignWin)
        for (int i = 0; i < MaxProtoFactionNum; ++i) {
            int uid = i + (faction_id * MaxProtoFactionNum);
            UNIT* cur = &Units[uid];
            if (!(cur->obsolete_factions & bm_fac)
            && cur->unit_flags & UNIT_UNK_100 && (!*MultiplayerActive || !is_human(faction_id))) {
                upgrade_prototypes(faction_id, uid);
            }
        }
    }
    // Modify diplomacy to skip social engineering choices dialog SOCIETY
    // while DiploWinState is active, otherwise use the previous conditions.
    // Original: DiploWinState is the diplomacy window; it is never open in the engine.
    if (faction_id == MapWin->cOwner && plr->diff_level > 1 && !*MultiplayerActive) {
        for (int sf = 0; sf < 4; ++sf) {
            for (int sm = 0; sm < 4; ++sm) {
                if (tech_id == SocialField[sf].soc_preq_tech[sm] && society_avail(sf, sm, faction_id)) {
                    int choice;
                    do {
                        parse_says(0, tech_str(tech_id), -1, -1);
                        parse_says(1, SocialField[sf].soc_name[sm] ? SocialField[sf].soc_name[sm] : "", -1, -1);
                        choice = X_pop_2("TUTOR", "SOCIETY", 0);
                        if (choice) {
                            social_select(faction_id);
                        }
                    } while (choice == 2);
                }
            }
        }
    }
    if ((Tech[tech_id].flags & TFLAG_REVEALS_MAP)
    && !(plr->player_flags & PFLAG_MAP_REVEALED)) {
        for (int i = 0; i < *MapAreaTiles; ++i) {
            (*MapTiles)[i].visibility |= bm_fac;
            if (faction_id) {
                (*MapTiles)[i].visible_items[faction_id - 1] = (*MapTiles)[i].items;
            }
        }
        for (int i = 0; i < *BaseCount; ++i) {
            Bases[i].visibility |= bm_fac;
            Bases[i].factions_pop_size_intel[faction_id] = Bases[i].pop_size;
        }
        plr->player_flags |= PFLAG_MAP_REVEALED;
        if (*CurrentTurn && faction_id == MapWin->cOwner) {
            parse_says(0, tech_str(tech_id), -1, -1);
            popp(ScriptFile, "SEEMAP2", 0, "native_sm.pcx", 0);
            draw_map(1);
        }
    }
    auto tech_owner = [](int cur_id, int plr_id) {
        return plr_id >= 0 && plr_id < MaxPlayerNum
            && cur_id >= 0 && cur_id <= TECH_TranT && TechOwners[cur_id] & (1 << plr_id);
    };
    tech_effects(faction_id);
    if (faction_id == MapWin->cOwner) {
        if (tech_id == Facility[FAC_BIOLOGY_LAB].preq_tech) {
            wp5_interlude(1);
        }
        if (tech_id == TECH_CentMed) {
            wp5_interlude(2);
        }
        if (project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt && faction_id > 0
        && Tech[TECH_HAL9000].preq_tech1 >= -1
        && (Tech[TECH_HAL9000].preq_tech2 >= -1 || Tech[TECH_HAL9000].preq_tech1 == -1)) {
            if (Tech[TECH_Matter].preq_tech1 >= -1
            && (Tech[TECH_Matter].preq_tech2 >= -1 || Tech[TECH_Matter].preq_tech1 == -1)) {
                if (tech_owner(TECH_HAL9000, faction_id)
                && tech_owner(TECH_Matter, faction_id)) {
                    wp5_interlude(15);
                    wp5_interlude(16);
                }
            }
        }
        if (tech_id == TECH_Psych && fac->is_alien()) {
            for (int i = 1; i < MaxPlayerNum; ++i) {
                if (faction_id != i && !MFactions[i].is_alien()
                && (plr->diplo_status[i] & DIPLO_COMMLINK)) {
                    wp5_interlude(36);
                }
            }
        }
    }
    // Fix: remove redundant conditions that check for tech_id == -1 that will never occur
    // TECH_TranT seems to be excluded by the following code on purpose
    auto is_valid_tech = [](int cur_id) {
        return cur_id >= 0 && cur_id < TECH_TranT
            && Tech[cur_id].preq_tech1 >= -1
            && (Tech[cur_id].preq_tech2 >= -1 || Tech[cur_id].preq_tech1 == -1);
    };
    int links_base_id = project_base(FAC_PLANETARY_DATALINKS);
    int links_owner_id;
    if (faction_id > 0 && links_base_id >= 0
    && (links_owner_id = Bases[links_base_id].faction_id, links_owner_id > 0)) {
        if (is_valid_tech(tech_id) && tech_owner(tech_id, faction_id)) {
            if (!tech_owner(tech_id, links_owner_id)) {
                int num = 0;
                for (int fc_id = 1; fc_id < MaxPlayerNum; ++fc_id) {
                    if (tech_owner(tech_id, fc_id)) {
                        ++num;
                    }
                }
                if (num >= PlanetaryDatalinksCount) {
                    tech_achieved(links_owner_id, tech_id, -1, 0);
                }
            }
        }
    }
    if (faction_id > 0 && is_valid_tech(tech_id) && tech_owner(tech_id, faction_id)) {
        for (int fc_id = 1; fc_id < MaxPlayerNum; ++fc_id) {
            if (MFactions[fc_id].rule_sharetech > 0 && !tech_owner(tech_id, fc_id)) {
                int num = 0;
                for (int tgt_id = 1; tgt_id < MaxPlayerNum; ++tgt_id) {
                    if (tech_owner(tech_id, tgt_id)) {
                        /*
                        Fix issue where TECHSHARE faction ability always skips the checks
                        for infiltration conditions while smac_only mode is activated.
                        Spying by probe team, pact, governor or Empath Guild is required.
                        Inverted RFLAG_TECHSHARE condition preserved for compatibility
                        but not possible without additional modding.
                        */
                        if (!(MFactions[fc_id].rule_flags & RFLAG_TECHSHARE)
                        || Factions[fc_id].diplo_status[tgt_id] & DIPLO_HAVE_INFILTRATOR
                        || Factions[fc_id].diplo_status[tgt_id] & DIPLO_PACT
                        || has_project(FAC_EMPATH_GUILD, fc_id)
                        || (fc_id == *GovernorFaction && !MFactions[tgt_id].is_alien())) {
                            ++num;
                        }
                    }
                }
                if (num >= MFactions[fc_id].rule_sharetech) {
                    tech_achieved(fc_id, tech_id, -2, 1);
                }
            }
        }
    }
    if (*CurrentTurn && is_first && Tech[tech_id].flags & TFLAG_SECRETS) {
        plr->player_flags |= PFLAG_FIRST_SECRETS;
        parse_says(0, tech_str(tech_id), -1, -1);
        parse_says(1, fac->adj_name_faction, -1, -1);
        snprintf(s_tech_buf, StrBufLen, "THESECRET%d", faction_id != MapWin->cOwner);
        X_pop(s_tech_buf, 0);
    }
}

int __cdecl tech_alt_cost(int tech_id, int faction_id) {
    MFaction* m = &MFactions[faction_id];
    int level = 1;
    int owners = 0;
    int our_techs = 0;

    if (tech_id >= 0) {
        level = tech_level(tech_id, 0);
        for (int i = 1; i < MaxPlayerNum; i++) {
            if (i != faction_id && is_alive(i) && has_tech(tech_id, i)
            && has_treaty(faction_id, i, DIPLO_COMMLINK)) {
                owners += (has_treaty(faction_id, i, DIPLO_PACT|DIPLO_HAVE_INFILTRATOR) ? 2 : 1);
            }
        }
    }
    for (int i = Tech_ID_First; i <= Tech_ID_Last; i++) {
        if (Tech[i].preq_tech1 != TECH_Disable && has_tech(i, faction_id)) {
            our_techs++;
        }
    }
    double cost_base = (5*level*level*level + 25*level*level + 20*our_techs);
    double cost_diff = (is_human(faction_id) ? 1.0 : conf.tech_cost_factor[clamp(*DiffLevel, 0, MaxDiffNum-1)] / 100.0)
        * clamp(our_techs + 6, 6, 16) / 16.0;

    double cost = cost_base
        * cost_diff
        * *MapAreaSqRoot / 56.0
        * max(1, m->rule_techcost) / 100.0
        * conf.tech_rate_modifier / 100.0
        * (*GameRules & RULES_TECH_STAGNATION ? conf.tech_stagnate_rate / 100.0 : 1.0)
        * (1.0 - 0.05*min(6, owners));

    debug("tech_cost %d %d base: %7.2f diff: %.2f cost: %7.2f "
    "level: %d our_techs: %d owners: %d tech: %2d %s\n",
    *CurrentTurn, faction_id, cost_base, cost_diff, cost,
    level, our_techs, owners, tech_id, tech_str(tech_id));

    return clamp((int)cost, 1, 99999999);
}

/*
Replace tech_val for some parts of the diplomacy dialog related to tech trading.
*/
int __cdecl tech_alt_val(int tech_id, int faction_id, int flag) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum || tech_id < 0 || tech_id >= MaxTechnologyNum) {
        return 0;
    }
    CTech& tech = Tech[tech_id];
    Faction& plr = Factions[faction_id];
    int def_value = plr.AI_fight + flag;
    for (int i = 1; i < MaxPlayerNum; i++) {
        if (i != faction_id && is_alive(i) && at_war(i, faction_id)) {
            def_value += (Factions[i].base_count > plr.base_count);
            def_value += (Factions[i].mil_strength_1 > plr.mil_strength_1);
            def_value += (Factions[i].best_weapon_value > plr.best_weapon_value);
        }
    }
    int level = clamp(tech_level(tech_id, 0), 1, 16);
    int owners = bit_count((uint32_t)TechOwners[tech_id]);
    int weights = (tech.flags & (TFLAG_IMPROVE_PROBE|TFLAG_INC_COMMERCE) ? 4 : 0)
        + 4*(tech_id == Weapon[WPN_TERRAFORMING_UNIT].preq_tech)
        + 4*(tech_id == Rules->tech_preq_allow_3_nutrients_sq)
        + 4*(tech_id == Rules->tech_preq_allow_3_minerals_sq)
        + 4*(tech_id == Rules->tech_preq_allow_3_energy_sq)
        + 4*(tech_id == Facility[FAC_SKY_HYDRO_LAB].preq_tech)
        + 4*(tech_id == Facility[FAC_ORBITAL_POWER_TRANS].preq_tech)
        + 4*(tech_id == Facility[FAC_NESSUS_MINING_STATION].preq_tech)
        + 4*(tech_id == Facility[FAC_ORBITAL_DEFENSE_POD].preq_tech)
        + 4*(tech_id == Facility[FAC_AEROSPACE_COMPLEX].preq_tech)
        + 4*(tech_id == Facility[FAC_HAB_COMPLEX].preq_tech
        && MFactions[faction_id].rule_population > 0)
        + 2*((plr.AI_growth + 1 + (plr.base_count < 8 + *MapAreaSqRoot/4)) * tech.AI_growth
        + (plr.AI_tech + 1 + (plr.SE_research > 0)) * tech.AI_tech
        + (plr.AI_wealth + 1 + (plr.SE_economy > 0)) * tech.AI_wealth
        + (plr.AI_power + 1 + (def_value > 2)) * tech.AI_power);

    for (int i = 0; i < MaxChassisNum; i++) {
        if (Chassis[i].preq_tech == tech_id) {
            weights += (Chassis[i].triad == TRIAD_AIR ? 8 : 4);
        }
    }
    for (int i = 0; i < MaxAbilityNum; i++) {
        if (Ability[i].preq_tech == tech_id && i != ABL_ID_SLOW && i != ABL_ID_CARRIER
        && i != ABL_ID_REPAIR && i != ABL_ID_DEEP_PRESSURE_HULL && i != ABL_ID_HEAVY_TRANSPORT) {
            weights += 4;
        }
    }
    for (int i = 0; i < MaxTerrainNum; i++) {
        if (Terraform[i].preq_tech == tech_id) {
            weights += 2;
        }
    }
    for (int i = 0; i < MaxWeaponNum; i++) {
        if (Weapon[i].preq_tech == tech_id
        && (!Weapon[i].offense_value || Weapon[i].offense_value >= plr.best_weapon_value)) {
            weights += 8;
        }
    }
    for (int i = 0; i < MaxArmorNum; i++) {
        if (Armor[i].preq_tech == tech_id && Armor[i].defense_value >= plr.best_armor_value) {
            weights += 4;
        }
    }
    int limit = max(100, *CurrentTurn * *BaseCount / 10);
    int value = (50*level + 8*level*level + clamp(*BaseCount / 4, 0, 80))
        * clamp(weights + 32, 32, 96) / 32
        * clamp(*MapAreaSqRoot, 30, 80) / 64
        * clamp(100 + (conf.tech_rate_modifier - 100)/2, 50, 200) / 100;
    value = (value - max(0, value - limit)/2);

    if (*GameRules & RULES_TECH_STAGNATION) {
        value = value * clamp(100 + (conf.tech_stagnate_rate - 100)/2, 50, 200) / 100;
    }
    if (owners > 1) {
        value = value * max(25, 100 - 15*owners) / 100;
    }
    return value;
}

/*
Probe team / TECHSTEAL acquisition (Thinker faction.cpp steal_tech).
Return Value: true when no tech was available and the world map was taken instead.
*/
int __cdecl steal_tech(int faction_id, int faction_id_tgt, int is_steal) {
    if (!faction_id || !faction_id_tgt
    || (*MultiplayerActive && is_human(faction_id) && faction_id != *CurrentPlayerFaction)) {
        return 0;
    }
    int tech_id = mod_tech_pick(faction_id, 0, faction_id_tgt, is_steal ? "STEALTECH" : "ACQUIRETECH");
    if (tech_id < 0) {
        tech_id = 9999;
    }
    // Original: multiplayer sends message_data(0x244C) instead.
    if (faction_id_tgt == *CurrentPlayerFaction && tech_id != 9999) {
        parse_says(0, get_noun(faction_id), -1, -1);
        s_tech_buf[0] = '\0';
        say_tech(s_tech_buf, tech_id, 0);
        parse_says(1, s_tech_buf, -1, -1);
        NetMsg_pop(NetMsg, "STOLETECH", 5000, 0, 0);
    }
    tech_achieved(faction_id, tech_id, faction_id_tgt, 0);
    if (!is_human(faction_id) && tech_id != 9999) {
        mod_bases_reset(-1, faction_id, 0);
    }
    return tech_id == 9999;
}

} // namespace engine
} // namespace alpha
