/* SPDX-License-Identifier: GPL-3.0-or-later
 * Comm-link proposal dialog model (WP8).
 *
 * A platform-free model of the diplomacy dialog between the human faction and a
 * computer player: the rival's mood, the proposals the treaty state allows, and
 * the answer to each proposal. A front end draws a DiploDialog and passes the
 * player's pick to diplo_dialog_propose(). install_ui_diplo() sets the engine's
 * DiploHooks::communicate so that comm-link contacts started by the engine
 * (encounter, enemy_diplomacy) open this dialog; the chooser callback stands in
 * for the player's clicks.
 *
 * The rules (what may be proposed, when the rival accepts, what changes) are in
 * engine/diplo_event.cpp: diplo_proposal_valid/accepted/apply.
 */
#pragma once

#include "engine.h"
#include "diplo.h"

namespace alpha {
namespace ui_diplo {

enum DiploAnswer {
    DA_NONE = -2,       // nothing proposed yet
    DA_INVALID = -1,    // not open in the current treaty state
    DA_REFUSED = 0,
    DA_ACCEPTED = 1,
};

struct DiploDialog {
    int player;                       // human faction
    int other;                        // computer player on the comm link
    uint32_t status;                  // Factions[player].diplo_status[other]
    int friction;                     // Factions[other].diplo_friction[player]
    int mood;                         // engine::get_mood(friction)
    int options[engine::DP_COUNT];    // proposals open now, in DiploProposal order
    int option_count;
    int last_proposal;                // -1 before the first proposal
    int last_answer;                  // DiploAnswer
    int turns;                        // proposals made in this dialog
    char reply[160];                  // the rival's last line (plain text)
};

/* Display names (English; the original script.txt texts are not used). */
const char* diplo_mood_name(int mood);
const char* diplo_proposal_label(int proposal);

/* Fill the dialog for player and other (state, mood, open proposals). */
void diplo_dialog_open(DiploDialog& d, int player, int other);
/* Make a proposal; on acceptance the treaty state changes. The dialog is
 * refreshed. Returns the DiploAnswer. */
int diplo_dialog_propose(DiploDialog& d, int proposal);

/* Picks the next proposal from an open dialog, or -1 to close the comm link. */
typedef int (*DiploChooser)(const DiploDialog& d, void* ctx);

/* Route the engine's communicate() through this dialog; chooser may be null
 * (the dialog opens and closes at once). At most max_proposals per contact. */
void install_ui_diplo(DiploChooser chooser, void* ctx, int max_proposals = 4);
void uninstall_ui_diplo();
/* The dialog of the last contact routed through communicate(). */
const DiploDialog& ui_diplo_last();

} // namespace ui_diplo
} // namespace alpha
