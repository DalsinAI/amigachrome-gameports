/* SPDX-License-Identifier: GPL-3.0-or-later
 * Social engineering panel: four categories by four models, which ones the
 * faction may choose (WP5 society_avail), the current choice and the
 * upheaval cost of each change. Layout is new restoration UI. The caller
 * fills the model and runs CMD_SOCIAL(row, col) on UIA_APPLY.
 */
#ifndef ALPHA_UI_SOCIAL_H
#define ALPHA_UI_SOCIAL_H
#include "ui_panel.h"
namespace alpha {
struct SocialPanelModel {
 bool valid;int faction;
 char categories[4][24];char models[4][4][24];
 bool available[4][4];int cost[4][4];  // upheaval cost of choosing this model now
 int current[4];int credits;
 int row,col;
};
/* Rect at least 480x240. */
size_t render_social_panel(const SocialPanelModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* UP/DOWN pick the category, LEFT/RIGHT the model, ENTER UIA_APPLY, BACK UIA_CLOSE. */
int social_panel_key(SocialPanelModel&,int key);
}
#endif
