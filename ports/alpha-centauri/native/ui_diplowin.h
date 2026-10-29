/* SPDX-License-Identifier: GPL-3.0-or-later
 * Comm-link dialog panel. The model is copied from WP8's ui_diplo::DiploDialog
 * (ui_diplo.h: mood, treaty state, open proposals, the rival's reply) by
 * play_session.cpp; this file only draws it and maps keys. UIA_APPLY runs
 * CMD_DIPLO(other, proposals[selected]). Layout is new restoration UI.
 */
#ifndef ALPHA_UI_DIPLOWIN_H
#define ALPHA_UI_DIPLOWIN_H
#include "ui_panel.h"
namespace alpha {
struct DiploPanelModel {
 bool valid;int player,other;
 char other_name[40],leader[48],mood[24],status[48];
 int count;int proposals[16];char labels[16][48];
 int selected;int last_answer;char reply[160];
};
/* Rect at least 420x260. */
size_t render_diplo_panel(const DiploPanelModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* UP/DOWN choose, ENTER UIA_APPLY, BACK UIA_CLOSE (closes the comm link). */
int diplo_panel_key(DiploPanelModel&,int key);
}
#endif
