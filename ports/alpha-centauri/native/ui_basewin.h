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

/* Production editing for the player's own base (engine-backed play mode).
 * The caller (play_session.cpp) fills the model from the engine: the items
 * the base may build (WP2 base_queue_valid_item), the queue and the hurry
 * quote (WP2 base_hurry_quote); keys map to commands. */
enum BaseItemKind { ITEM_FACILITY=0,ITEM_PROJECT,ITEM_COMBAT,ITEM_COLONY,ITEM_FORMER,ITEM_OTHER_UNIT };
struct BaseProductionItem { int id,kind;char name[32];int cost,turns; };
struct BaseProductionModel {
 bool valid;int base;char name[26];
 int current;char current_name[32];int minerals,cost,surplus,turns;
 int queue_count;int queue[10];char queue_names[10][32];
 int hurry_cost,credits;bool hurried;
 int count;BaseProductionItem items[96];int selected;
};
enum BaseEditAction { BASE_EDIT_NONE=0,BASE_EDIT_MOVED,BASE_EDIT_SET,BASE_EDIT_QUEUE_ADD,BASE_EDIT_QUEUE_REMOVE,BASE_EDIT_HURRY,BASE_EDIT_CLOSE };
/* Choice list with the current item, queue and hurry line. Rect at least 360x300. */
size_t render_base_production(const BaseProductionModel&,const UiFrame&,const UiRect&,UiTextItem* items,size_t capacity);
/* Character keys: 8/2 select, RETURN set production, A add to the queue,
 * X remove the last queue entry, H hurry, ESC close. */
int base_production_key(BaseProductionModel&,int c);
}
#endif
