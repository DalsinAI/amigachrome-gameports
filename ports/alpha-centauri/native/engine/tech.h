/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP5 research and social engineering: declarations beyond engine_wp.h.
 * Names and signatures follow Thinker Mod src/tech.h, src/faction.h and the
 * function-pointer typedefs of src/engine.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * Routines that only exist in terranx.exe (tech_pick, tech_rate, social_set,
 * social_select) are written fresh from the documented original rules; see
 * ENGINE-WP5.md. Include after engine.h.
 */
#pragma once

namespace alpha {
namespace engine {

/* ---- tech.cpp (Thinker tech.cpp) ---- */
const char* tech_str(int tech_id);
void __cdecl say_tech(char* output, int tech_id, int incl_category);
int __cdecl tech_category(int tech_id);
int __cdecl tech_is_preq(int preq_tech_id, int parent_tech_id, int range);
int __cdecl tech_alt_cost(int tech_id, int faction_id);
int __cdecl tech_alt_val(int tech_id, int faction_id, int flag);

/* terranx.exe routines, written fresh (tech.cpp). */
int __cdecl tech_pick(int faction_id, int flag, int other_faction_id, const char* label);
int __cdecl tech_rate(int faction_id);
int __cdecl tech_selection(int faction_id);
int __cdecl steal_tech(int faction_id, int faction_id_tgt, int is_steal);

/* Original-game interface state used by tech_achieved/tech_advance. Not saved.
 * SkipTechScreenA suppresses the ranking update and the discovery screens
 * while techs are granted outside play (setup, time warp). */
extern int* SkipTechScreenA;
extern int* SkipTechScreenB;

/* ---- Human research choice (tech_pick for a human player) ----
 * The candidate list follows the original directed-human pool: tech_avail
 * candidates scanned by category (growth, tech, wealth, power) and then by id;
 * after the first offer, an id with (id - tech_ranking + faction) % 3 == 0 is
 * withheld (ids 0..6 are exempt while tech_ranking is 0). For trade/steal
 * (other_faction_id >= 0) the list is every tech_avail tech the other faction
 * owns, in id order. */
struct TechChoice {
    int faction_id;
    int other_faction_id;     // -1 for normal research
    int flag;                 // original tech_pick flag
    const char* label;        // script label (ACQUIRETECH, STEALTECH, ...) or null
    int count;
    int ids[MaxTechnologyNum];
    int categories[MaxTechnologyNum];
};
/* Return the chosen tech id (one of choice.ids) or -1 to cancel. */
typedef int (*TechChooser)(const TechChoice& choice, void* context);
/* Install the UI chooser (null restores the default). Without one, the choice
 * is a popup through the event sink (label, numbers[] = first 10 ids,
 * answer = list index; -1 cancels). Returns the previous chooser. */
TechChooser set_tech_chooser(TechChooser chooser, void* context);
/* Fill the normal human research pool (other_faction_id < 0) or the
 * trade/steal list. Returns the count. */
int research_pool(int faction_id, int other_faction_id, TechChoice* out);
/* Techs another faction could give this one by trade (tech_avail for
 * faction_id and owned by other). Returns the count written to ids. */
int tech_tradeable(int faction_id, int other_faction_id, int* ids, int max_ids);

/* Manual research change from the research screen (Console_set_tech):
 * an existing nonnegative target with nonzero accumulated labs needs
 * confirmation; a confirmed change of an existing target halves the labs. */
enum ResearchChange {
    RESEARCH_CHANGED = 0,
    RESEARCH_SET = 1,                  // there was no target before
    RESEARCH_SAME = 2,                 // already the target; nothing changed
    RESEARCH_NEEDS_CONFIRMATION = 3,   // nothing changed; ask the player first
    RESEARCH_INVALID = 4,              // not tech_avail for this faction
};
int research_change(int faction_id, int tech_id, bool confirmed);
bool research_change_needs_confirmation(int faction_id);

/* ---- social.cpp (Thinker faction.cpp social engineering) ---- */
void __cdecl compute_faction_modifiers(int faction_id);
/* terranx.exe routines, written fresh. social_set applies the pending social
 * choices to the pending effect values (SE_*_pending) and the commerce bonus.
 * social_select is the human Social Engineering screen; the engine asks the
 * installed SocialChooser (default: event-sink popup "SOCIALSELECT" that keeps
 * the current choices). */
void __cdecl social_set(int faction_id);
void __cdecl social_select(int faction_id);
/* Change one social category for a faction, paying the upheaval cost from
 * energy credits like the original screen. Returns the cost paid, or -1 when
 * the model is not available or the faction cannot afford it. */
int social_change(int faction_id, int category, int model);
typedef bool (*SocialChooser)(int faction_id, CSocialCategory* choices, void* context);
SocialChooser set_social_chooser(SocialChooser chooser, void* context);

} // namespace engine
} // namespace alpha
