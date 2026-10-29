/* SPDX-License-Identifier: GPL-3.0-or-later
 * Research panel: the current research goal and progress, and the
 * technologies the player may research next (the engine's human research
 * pool, WP5 research_pool). Layout is new restoration UI. The caller fills the
 * model (play_session.cpp) and runs CMD_RESEARCH with the chosen id.
 */
#ifndef ALPHA_UI_RESEARCH_H
#define ALPHA_UI_RESEARCH_H
#include "ui_panel.h"
namespace alpha {
struct ResearchPanelModel {
 bool valid;int faction;
 int current;char current_name[40];
 int accumulated,cost,labs,known;
 int count;int ids[32];char names[32][40];char categories[32][12];
 int selected;
 bool confirm;   // the last APPLY needs confirmation (progress is halved)
};
/* Panel with the goal, progress bar and choice list. Rect at least 360x260. */
size_t render_research_panel(const ResearchPanelModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* UIK_* key: UP/DOWN move, ENTER returns UIA_APPLY (ids[selected]), BACK UIA_CLOSE. */
int research_panel_key(ResearchPanelModel&,int key);
}
#endif
