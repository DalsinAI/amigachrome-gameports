/* SPDX-License-Identifier: GPL-3.0-or-later
 * Battle panel: before an attack the odds preview (WP4 engine_attack_odds,
 * nothing changes), after it the result (WP4 BattleResult: outcome,
 * strengths, hit points, rounds and the modifier lines). Layout is new
 * restoration UI.
 */
#ifndef ALPHA_UI_BATTLE_H
#define ALPHA_UI_BATTLE_H
#include "ui_panel.h"
namespace alpha {
struct BattlePanelModel {
 bool valid,preview;int x,y;
 char attacker[32],defender[32],attacker_owner[24],defender_owner[24];
 int odds;   // engine_attack_odds: offense*8/(defense+1), 0 = cannot attack
 int outcome;char outcome_text[48];
 int offense,defense,hp_atk_before,hp_def_before,hp_atk_after,hp_def_after,rounds,attacker_hits,defender_hits,killed;
 bool promoted_atk,promoted_def,base_captured,base_destroyed;
 int mod_count[2];char mods[2][4][48];
};
/* Rect at least 420x220. */
size_t render_battle_panel(const BattlePanelModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* Preview: ENTER UIA_APPLY (attack), BACK UIA_CLOSE. Result: ENTER/BACK UIA_CLOSE. */
int battle_panel_key(const BattlePanelModel&,int key);
}
#endif
