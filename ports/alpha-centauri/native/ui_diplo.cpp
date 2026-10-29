/* SPDX-License-Identifier: GPL-3.0-or-later
 * Comm-link proposal dialog model (WP8); see ui_diplo.h.
 */
#include "ui_diplo.h"

namespace alpha {
namespace ui_diplo {

using namespace alpha::engine;

namespace {

struct UiDiploState {
    DiploChooser chooser = nullptr;
    void* ctx = nullptr;
    int max_proposals = 4;
    DiploDialog last = {};
};

UiDiploState s_ui;

bool valid_faction(int faction_id) {
    return faction_id > 0 && faction_id < MaxPlayerNum;
}

void refresh(DiploDialog& d) {
    d.status = Factions[d.player].diplo_status[d.other];
    d.friction = Factions[d.other].diplo_friction[d.player];
    d.mood = get_mood(d.friction);
    d.option_count = 0;
    for (int p = 0; p < DP_COUNT; p++) {
        if (diplo_proposal_valid(d.player, d.other, p)) {
            d.options[d.option_count++] = p;
        }
    }
    for (int i = d.option_count; i < DP_COUNT; i++) {
        d.options[i] = -1;
    }
}

int ui_communicate(int faction_id, int faction_id_2, int UNUSED(flag)) {
    // The human is whichever side is human; the engine calls with the human first.
    int player = faction_id;
    int other = faction_id_2;
    if (!is_human(player) && is_human(other)) {
        player = faction_id_2;
        other = faction_id;
    }
    DiploDialog& d = s_ui.last;
    diplo_dialog_open(d, player, other);
    Factions[player].diplo_spoke[other] = *CurrentTurn;
    for (int n = 0; s_ui.chooser && n < s_ui.max_proposals; n++) {
        int proposal = s_ui.chooser(d, s_ui.ctx);
        if (proposal < 0) {
            break;
        }
        diplo_dialog_propose(d, proposal);
    }
    DiploStateA[other] = 0;
    return d.last_answer == DA_ACCEPTED ? 1 : 0;
}

} // namespace

const char* diplo_mood_name(int mood) {
    static const char* const names[] = {
        "Magnanimous", "Solicitous", "Cooperative", "Noncommittal", "Ambivalent",
        "Obstinate", "Quarrelsome", "Belligerent", "Seething",
    };
    if (mood < MOOD_MAGNANIMOUS || mood > MOOD_SEETHING) {
        return "";
    }
    return names[mood];
}

const char* diplo_proposal_label(int proposal) {
    switch (proposal) {
    case DP_TRUCE: return "Propose a truce";
    case DP_TREATY: return "Propose a treaty of friendship";
    case DP_PACT: return "Propose a pact of brotherhood";
    case DP_END_PACT: return "Renounce our pact";
    case DP_VENDETTA: return "Declare vendetta";
    default: return "";
    }
}

void diplo_dialog_open(DiploDialog& d, int player, int other) {
    d = DiploDialog();
    d.last_proposal = -1;
    d.last_answer = DA_NONE;
    for (int i = 0; i < DP_COUNT; i++) {
        d.options[i] = -1;
    }
    if (!valid_faction(player) || !valid_faction(other) || player == other) {
        d.player = 0;
        d.other = 0;
        return;
    }
    d.player = player;
    d.other = other;
    refresh(d);
    snprintf(d.reply, sizeof(d.reply), "%s is %s.", get_noun(other), diplo_mood_name(d.mood));
}

int diplo_dialog_propose(DiploDialog& d, int proposal) {
    d.last_proposal = proposal;
    d.turns++;
    if (!d.player || !diplo_proposal_valid(d.player, d.other, proposal)) {
        d.last_answer = DA_INVALID;
        snprintf(d.reply, sizeof(d.reply), "That is not on the table.");
        return d.last_answer;
    }
    if (diplo_proposal_accepted(d.player, d.other, proposal)) {
        diplo_proposal_apply(d.player, d.other, proposal);
        d.last_answer = DA_ACCEPTED;
        snprintf(d.reply, sizeof(d.reply), "%s agrees: %s.", get_noun(d.other),
            diplo_proposal_label(proposal));
    } else {
        d.last_answer = DA_REFUSED;
        snprintf(d.reply, sizeof(d.reply), "%s refuses.", get_noun(d.other));
    }
    refresh(d);
    return d.last_answer;
}

void install_ui_diplo(DiploChooser chooser, void* ctx, int max_proposals) {
    s_ui.chooser = chooser;
    s_ui.ctx = ctx;
    s_ui.max_proposals = max(0, max_proposals);
    diplo_hooks().communicate = ui_communicate;
}

void uninstall_ui_diplo() {
    s_ui.chooser = nullptr;
    s_ui.ctx = nullptr;
    diplo_hooks().communicate = nullptr;
}

const DiploDialog& ui_diplo_last() {
    return s_ui.last;
}

} // namespace ui_diplo
} // namespace alpha
