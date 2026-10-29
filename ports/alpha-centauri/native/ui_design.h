/* SPDX-License-Identifier: GPL-3.0-or-later
 * Design workshop panel: chassis, weapon or module, armor, reactor and one
 * special ability from the components the faction has (WP5 rules tables and
 * has_tech). The preview name and cost come from the engine
 * (Session::design_preview, WP5 mod_name_proto/proto_cost); UIA_APPLY runs
 * CMD_DESIGN, which creates the prototype with WP5 create_proto. Layout is new
 * restoration UI.
 */
#ifndef ALPHA_UI_DESIGN_H
#define ALPHA_UI_DESIGN_H
#include "ui_panel.h"
namespace alpha {
enum DesignRow { DESIGN_CHASSIS=0,DESIGN_WEAPON,DESIGN_ARMOR,DESIGN_REACTOR,DESIGN_ABILITY,DESIGN_ROWS };
struct DesignChoice { int id;char name[32]; };
struct DesignPanelModel {
 bool valid;int faction;
 int counts[DESIGN_ROWS];DesignChoice choices[DESIGN_ROWS][32];int selected[DESIGN_ROWS];
 int row;
 bool preview_ok;char preview_name[40];int preview_cost;
 int last_unit;   // unit id created by the last CMD_DESIGN, -1 none
};
/* Selected component ids: chassis, weapon, armor, reactor, ability flags. */
void design_selection(const DesignPanelModel&,int& chassis,int& weapon,int& armor,int& reactor,int& abilities);
/* Rect at least 420x260. */
size_t render_design_panel(const DesignPanelModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* UP/DOWN row, LEFT/RIGHT component (UIA_MOVED: refresh the preview), ENTER UIA_APPLY, BACK UIA_CLOSE. */
int design_panel_key(DesignPanelModel&,int key);
}
#endif
