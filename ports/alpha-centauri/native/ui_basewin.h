/* SPDX-License-Identifier: GPL-3.0-or-later
 * Base radius order (21 tiles) from Thinker path.h TableOffset, MIT,
 * pin dabe62f2; see THINKER-LICENSE.txt. Layout is new restoration UI.
 */
#ifndef ALPHA_UI_BASEWIN_H
#define ALPHA_UI_BASEWIN_H
#include "original_world.h"
#include "rules_text.h"
#include "ui_panel.h"
namespace alpha {
/* Fresh accounting supplied by the caller (e.g. from world_base_accounting);
 * without it the saved-turn intake/consumption/surplus fields are shown. */
struct BaseWindowForecast { bool valid;int32_t nutrient[3],mineral[3],energy[3]; /* intake, consumption/upkeep/loss, surplus */ };
struct BaseWindowTile { int x,y;bool present,worked,ocean,rocky,fungus,explored; };
struct BaseWindowModel {
 char name[26];int32_t population,faction,specialists,talents,drones;
 BaseWindowTile tiles[21];unsigned worked_count;
 int32_t nutrient[3],mineral[3],energy[3];int32_t stored_nutrients,stored_minerals;
 int32_t economy,labs,psych;bool fresh;
 unsigned queue_count;char queue[10][40];
 unsigned facility_count;char facilities[64][40];
};
/* Read-only snapshot of one base. facilities may be null (numbered names).
 * Viewer -1 or the owner sees everything; other factions see unexplored tiles
 * as such. False on a bad index. */
bool build_base_window(const OriginalWorld&,size_t base,const FacilityDefinitions* facilities,
 const BaseWindowForecast* forecast,int viewer,BaseWindowModel&);
/* Panel: radius map with worked tiles, three resource bars, then text items
 * for the header, resources, queue and facilities. Rect at least 360x300. */
size_t render_base_window(const BaseWindowModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
}
#endif
