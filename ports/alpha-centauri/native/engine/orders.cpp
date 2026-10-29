/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 order helpers and the player order entry points used by the UI.
 *
 * say_loc is ported from Thinker Mod src/gui.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * supply_options, quick_zoc, base_on_sea, port_to_port, radius_move_2 and
 * veh_ready exist only in terranx.exe and no upstream has their source: they
 * are new code that follows how Thinker's ported callers use them (see
 * ENGINE-WP3.md). The order_* functions are new: the original executable did
 * this work in its Console keyboard handlers, which are UI code.
 */
#include "engine.h"
#include "veh_action.h"
#include "veh_action_ui.h"
#include "wp3_deps.h"

namespace alpha {
namespace engine {

using namespace wp3_ui;

void __cdecl say_loc(char* dest, int x, int y, int a4, int a5, int a6)
{
    int base_id = -1;
    MAP* sq;
    if ((sq = mapsq(x, y)) && sq->is_base()
    && (*GameState & STATE_SCENARIO_EDITOR || sq->is_visible(MapWin->cOwner))) {
        base_id = base_at(x, y);
    }
    const char* prefix = "";
    const char* prefix_space = "";
    if (a4 != 0 && base_id < 0) {
        base_id = base_find_3(x, y, -1, -1, -1, MapWin->cOwner);
        if (base_id >= 0) {
            a6 = 0;
            prefix = label_get(TL_Near);
            prefix_space = " ";
        }
    }
    const char* base_name = "";
    const char* trail_space = "";
    if (base_id >= 0) {
        if (a6) {
            prefix = label_get(TL_At);
            prefix_space = " ";
        }
        base_name = Bases[base_id].name;
        if (a5) {
            trail_space = " ";
        }
    }
    size_t prev = strnlen(dest, StrBufLen);
    size_t len = StrBufLen - prev;
    if (a5 == 1 || (a5 == 2 && base_id < 0)) {
        snprintf(dest + prev, len, "%s%s%s%s(%d, %d)",
            prefix, prefix_space, base_name, trail_space, x, y);
    } else {
        snprintf(dest + prev, len, "%s%s%s%s",
            prefix, prefix_space, base_name, trail_space);
    }
}

/* Mineral rows to minerals for a faction (WP2's mod_cost_factor; 10 while
 * that package is a stub that returns 0). */
static int mineral_row_cost(int faction_id, int base_id) {
    int factor = mod_cost_factor(faction_id, RSC_MINERAL, base_id);
    return factor > 0 ? factor : 10;
}

/*
New code for terranx supply_options (0x593A00): a supply unit moving into a
friendly base. order_veh calls it twice: without VehAttackFlags & 2 (move
check) a zero result cancels the move, with VehAttackFlags & 2 (execute) a
nonzero result means the unit was used up and the move ends there.
Human players get the SUPPLYOPTIONS popup: answer 1 = help build (the
crawler's full mineral value goes into current production and the unit is
disbanded), anything else = just move in. The AI always moves in (Thinker's
AI handles its convoys itself).
*/
int __cdecl supply_options(int veh_id, int base_id) {
    if (!(*VehAttackFlags & 2)) {
        return 1;
    }
    VEH* veh = &Vehs[veh_id];
    BASE* base = &Bases[base_id];
    int faction_id = veh->faction_id;
    if (!is_human(faction_id) || base->faction_id != faction_id
    || !(*VehAttackFlags & 1) || faction_id != MapWin->cOwner) {
        return 0;
    }
    parse_says(0, veh->name(), -1, -1);
    parse_says(1, base->name, -1, -1);
    if (popp(ScriptFile, "SUPPLYOPTIONS", 0, "supply_sm.pcx", 0) != 1) {
        return 0;
    }
    int minerals = mod_veh_cost(veh->unit_id, base_id, nullptr) * mineral_row_cost(faction_id, base_id);
    base->minerals_accumulated += minerals;
    base->minerals_accumulated_2 = base->minerals_accumulated;
    parse_num(0, minerals);
    NetMsg_pop(NetMsg, "SUPPLYHELP", 5000, 0, 0);
    kill(veh_id);
    return 1;
}

/*
New code for terranx quick_zoc (0x593830): the location of an enemy unit that
exerts a zone of control on (x1, y1) for faction_id, preferring one adjacent
to (x2, y2). Used for the ZOC tutorial message only.
*/
void __cdecl quick_zoc(int x1, int y1, int faction_id, int x2, int y2, int* zx, int* zy) {
    *zx = -1;
    *zy = -1;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < 8; i++) {
            int px = wrap(x1 + BaseOffsetX[i]);
            int py = y1 + BaseOffsetY[i];
            MAP* sq = mapsq(px, py);
            if (!sq) {
                continue;
            }
            int owner = sq->veh_who();
            if (owner >= 0 && owner != faction_id && !has_pact(faction_id, owner)
            && (pass || map_range(px, py, x2, y2) <= 1)) {
                *zx = px;
                *zy = py;
                return;
            }
        }
    }
}

/*
New code for terranx base_on_sea (0x50DE50): nonzero when the base is next to
ocean of the given sea region (or in it, for sea bases).
*/
int __cdecl base_on_sea(int base_id, int sea_region) {
    if (base_id < 0 || base_id >= *BaseCount) {
        return 0;
    }
    BASE* base = &Bases[base_id];
    for (int i = 0; i < 9; i++) {
        int px = wrap(base->x + TableOffsetX[i]);
        int py = base->y + TableOffsetY[i];
        MAP* sq = mapsq(px, py);
        if (sq && is_ocean(sq) && sq->region == sea_region) {
            return 1;
        }
    }
    return 0;
}

/*
New code for terranx port_to_port (0x50E160): nonzero when both bases touch a
common sea region.
*/
int __cdecl port_to_port(int base_id_1, int base_id_2) {
    if (base_id_1 < 0 || base_id_1 >= *BaseCount) {
        return 0;
    }
    BASE* base = &Bases[base_id_1];
    for (int i = 0; i < 9; i++) {
        int px = wrap(base->x + TableOffsetX[i]);
        int py = base->y + TableOffsetY[i];
        MAP* sq = mapsq(px, py);
        if (sq && is_ocean(sq) && base_on_sea(base_id_2, sq->region)) {
            return 1;
        }
    }
    return 0;
}

/*
New code for terranx radius_move_2 (0x5A65D0): the TableOffset index (below
range) that leads from (x1, y1) to (x2, y2), or -1.
*/
int __cdecl radius_move_2(int x1, int y1, int x2, int y2, int range) {
    range = clamp(range, 0, TableRange[MaxTableRange]);
    for (int i = 0; i < range; i++) {
        if (wrap(x1 + TableOffsetX[i]) == x2 && y1 + TableOffsetY[i] == y2) {
            return i;
        }
    }
    return -1;
}

/*
New code for terranx veh_ready (0x5C1B70): the unit still has movement points.
*/
int __cdecl veh_ready(int veh_id) {
    return veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent > 0;
}

/* ------------------------------------------------------------------------- */
/* Player orders (new code). Every move goes through order_veh, so movement  */
/* points, ZOC, combat, pods and monoliths follow the ported rules.          */
/* ------------------------------------------------------------------------- */

static bool valid_veh(int veh_id) {
    return veh_id >= 0 && veh_id < *VehCount;
}

static int moves_left(int veh_id) {
    return veh_speed(veh_id, 0) - Vehs[veh_id].moves_spent;
}

int order_move(int veh_id, int offset) {
    if (!valid_veh(veh_id) || offset < 0 || offset >= 8 || moves_left(veh_id) <= 0) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    if (veh->order == ORDER_SENTRY_BOARD && veh_jail(veh_id)) {
        return 0;
    }
    veh->order = ORDER_NONE;
    veh->state &= ~VSTATE_WORKING;
    return order_veh(veh_id, offset, 3);
}

/* Run a go-to/road-to order until it ends, the unit stops moving or it runs
 * out of movement points. Returns the number of steps taken. */
static int run_standing_order(int veh_id) {
    int steps = 0;
    for (int i = 0; i < PathLimit && valid_veh(veh_id); i++) {
        VEH* veh = &Vehs[veh_id];
        uint8_t order = veh->order;
        if (order != ORDER_MOVE_TO && order != ORDER_ROAD_TO && order != ORDER_MAGTUBE_TO
        && order != ORDER_AI_GO_TO && order != ORDER_MOVE) {
            break;
        }
        if (moves_left(veh_id) <= 0) {
            break;
        }
        int x = veh->x;
        int y = veh->y;
        int spent = veh->moves_spent;
        int count = *VehCount;
        action(veh_id);
        if (*VehCount != count || !valid_veh(veh_id)) {
            break; // unit killed or merged
        }
        if (Vehs[veh_id].x != x || Vehs[veh_id].y != y) {
            steps++;
        } else if (Vehs[veh_id].moves_spent == spent && Vehs[veh_id].order == order) {
            break; // no progress
        }
    }
    return steps;
}

int order_go_to(int veh_id, int x, int y) {
    if (!valid_veh(veh_id) || !mapsq(x, y)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    if (veh->x == x && veh->y == y) {
        return 0;
    }
    veh->order = ORDER_MOVE_TO;
    veh->waypoint_x[0] = x;
    veh->waypoint_y[0] = y;
    veh->status_icon = 'G';
    veh->state &= ~VSTATE_WORKING;
    if (Path_move(veh_id, veh->faction_id) < 0) {
        veh->order = ORDER_NONE;
        return 0;
    }
    run_standing_order(veh_id);
    return 1;
}

int order_road_to(int veh_id, int x, int y, bool magtube) {
    if (!valid_veh(veh_id) || !mapsq(x, y) || !Vehs[veh_id].is_former()
    || Vehs[veh_id].triad() != TRIAD_LAND) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    veh->order = magtube ? ORDER_MAGTUBE_TO : ORDER_ROAD_TO;
    veh->waypoint_x[0] = x;
    veh->waypoint_y[0] = y;
    veh->status_icon = magtube ? 't' : 'r';
    run_standing_order(veh_id);
    return 1;
}

/* Terraform legality: the checks the original's Console::terraform menu made
 * before calling action_terraform (former unit, technology, ocean/land item,
 * tile state). */
static bool can_terraform(int veh_id, int item_id) {
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (!sq || item_id < FORMER_FARM || item_id > FORMER_LEVEL_TERRAIN || !veh->is_former()) {
        return false;
    }
    if (sq->is_base() && item_id != FORMER_ROAD && item_id != FORMER_MAGTUBE) {
        return false;
    }
    bool ocean = is_ocean(sq);
    if (!terrain_avail((FormerItem)item_id, ocean, veh->faction_id)) {
        return false;
    }
    if (ocean && veh->triad() != TRIAD_SEA && !has_abil(veh->unit_id, ABL_AMPHIBIOUS)) {
        return false;
    }
    if (!ocean && veh->triad() == TRIAD_SEA) {
        return false;
    }
    uint32_t items = sq->items;
    switch (item_id) {
    case FORMER_REMOVE_FUNGUS:
        return items & BIT_FUNGUS;
    case FORMER_PLANT_FUNGUS:
        return !(items & BIT_FUNGUS);
    case FORMER_SOIL_ENR:
        return (items & BIT_FARM) && !(items & BIT_SOIL_ENRICHER);
    case FORMER_MAGTUBE:
        return (items & BIT_ROAD) && !(items & BIT_MAGTUBE);
    case FORMER_ROAD:
        return !ocean && !(items & BIT_MAGTUBE);
    case FORMER_LEVEL_TERRAIN:
        return sq->rocky_level() > 0;
    default:
        return !(items & Terraform[item_id].bit) || item_id == FORMER_RAISE_LAND
            || item_id == FORMER_LOWER_LAND || item_id == FORMER_FARM;
    }
}

int order_terraform(int veh_id, int item_id) {
    if (!valid_veh(veh_id) || !can_terraform(veh_id, item_id)) {
        return 0;
    }
    if (item_id == FORMER_FARM && (mapsq(Vehs[veh_id].x, Vehs[veh_id].y)->items & BIT_FARM)
    && !has_tech(Terraform[FORMER_SOIL_ENR].preq_tech, Vehs[veh_id].faction_id)) {
        return 0;
    }
    const char* letter = Order[item_id + VehOrderFormerFirst].letter;
    Vehs[veh_id].status_icon = (letter ? letter[0] : 0);
    action_terraform(veh_id, item_id, 1);
    return 1;
}

int order_build_base(int veh_id, const char* name) {
    if (!valid_veh(veh_id)) {
        return -1;
    }
    VEH* veh = &Vehs[veh_id];
    MAP* sq = mapsq(veh->x, veh->y);
    if (!sq || !Units[veh->unit_id].is_colony() || sq->is_base()) {
        return -1;
    }
    if ((veh->triad() == TRIAD_SEA) != is_ocean(sq) && veh->triad() != TRIAD_AIR) {
        return -1;
    }
    if (veh->y < 1 || veh->y >= *MapAreaY - 1) {
        return -1; // poles
    }
    for (int i = 1; i < 9; i++) {
        int px = wrap(veh->x + TableOffsetX[i]);
        int py = veh->y + TableOffsetY[i];
        if (base_at(px, py) >= 0) {
            return -1; // adjacent to another base
        }
    }
    return action_build(veh_id, name);
}

int order_sentry(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    sleep(veh_id);
    veh->status_icon = 'L';
    if (veh->triad() == TRIAD_LAND) {
        for (int i = veh_top(veh_id); i >= 0; i = Vehs[i].next_veh_id_stack) {
            if (i != veh_id && Vehs[i].faction_id == veh->faction_id
            && veh_cargo(i) > veh_cargo_loaded(i)) {
                veh->waypoint_x[0] = i;
                break;
            }
        }
    }
    veh_skip(veh_id);
    return 1;
}

int order_hold(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    veh->order = ORDER_HOLD;
    veh->waypoint_x[0] = -1;
    veh->waypoint_y[0] = 0;
    veh->status_icon = 'H';
    veh_skip(veh_id);
    return 1;
}

int order_wake(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    veh_wake(veh_id);
    Vehs[veh_id].status_icon = '-';
    return 1;
}

int order_skip(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    veh_skip(veh_id);
    return 1;
}

/*
Disband. Inside an own base half of the unit's mineral value goes into the
current production (original rule); anywhere else the unit is just removed.
*/
int order_disband(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    int base_id = base_at(veh->x, veh->y);
    if (base_id >= 0 && Bases[base_id].faction_id == veh->faction_id) {
        int minerals = mod_veh_cost(veh->unit_id, base_id, nullptr)
            * mineral_row_cost(veh->faction_id, base_id) / 2;
        Bases[base_id].minerals_accumulated += minerals;
        Bases[base_id].minerals_accumulated_2 = Bases[base_id].minerals_accumulated;
    }
    kill(veh_id);
    return 1;
}

int order_set_home(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    int base_id = base_at(veh->x, veh->y);
    if (base_id < 0 || Bases[base_id].faction_id != veh->faction_id
    || veh->home_base_id == base_id) {
        return 0;
    }
    veh->home_base_id = base_id;
    return 1;
}

int order_return_home(int veh_id) {
    if (!valid_veh(veh_id) || !action_home(veh_id, 0)) {
        return 0;
    }
    if (valid_veh(veh_id) && Vehs[veh_id].order == ORDER_MOVE_TO) {
        Vehs[veh_id].status_icon = 'G';
        run_standing_order(veh_id);
    }
    return 1;
}

/* Supply crawler convoy (Thinker set_convoy without the AI bookkeeping). */
int order_convoy(int veh_id, int resource) {
    if (!valid_veh(veh_id) || !Vehs[veh_id].is_supply()
    || resource < RES_NUTRIENT || resource > RES_ENERGY) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    if (base_at(veh->x, veh->y) >= 0 || base_find_2(veh->x, veh->y, veh->faction_id) < 0) {
        return 0;
    }
    veh->order_auto_type = resource - 1;
    veh->order = ORDER_CONVOY;
    veh->status_icon = 'C';
    veh_skip(veh_id);
    return 1;
}

/*
Upgrade one unit (Thinker veh_upgrade with the part_upgrade conditions): in
an own base, not moved, not damaged, same owner and triad; costs
10 * mod_upgrade_cost energy credits.
*/
int order_upgrade(int veh_id, int new_unit_id) {
    if (!valid_veh(veh_id) || new_unit_id < 0 || new_unit_id >= MaxProtoNum) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    int faction_id = veh->faction_id;
    int old_unit_id = veh->unit_id;
    Faction* f = &Factions[faction_id];
    if (new_unit_id == old_unit_id || !Units[new_unit_id].is_active()
    || (new_unit_id >= MaxProtoFactionNum && new_unit_id / MaxProtoFactionNum != faction_id)
    || Units[new_unit_id].triad() != Units[old_unit_id].triad()
    || veh->moves_spent || veh->damage_taken) {
        return 0;
    }
    int base_id = base_at(veh->x, veh->y);
    if (base_id < 0 || Bases[base_id].faction_id != faction_id) {
        return 0;
    }
    int cost = 10 * mod_upgrade_cost(faction_id, new_unit_id, old_unit_id);
    if (f->energy_credits < cost) {
        return 0;
    }
    f->energy_credits -= cost;
    f->units_active[new_unit_id]++;
    f->units_active[old_unit_id]--;
    veh->unit_id = new_unit_id;
    int morale_diff = (has_abil(new_unit_id, ABL_TRAINED) != 0)
        - (has_abil(old_unit_id, ABL_TRAINED) != 0);
    veh->morale = clamp(veh->morale + morale_diff, 0, 6);
    veh_skip(veh_id);
    draw_tile(veh->x, veh->y, 2);
    return 1;
}

int order_airdrop(int veh_id, int x, int y) {
    if (!valid_veh(veh_id) || !mapsq(x, y)) {
        return 0;
    }
    VEH* veh = &Vehs[veh_id];
    if (!has_abil(veh->unit_id, ABL_DROP_POD) || veh->moves_spent
    || (veh->state & VSTATE_MADE_AIRDROP)) {
        return 0;
    }
    MAP* sq = mapsq(veh->x, veh->y);
    if (!sq || (!sq->is_base() && !(sq->items & BIT_AIRBASE))) {
        return 0;
    }
    return action_airdrop(veh_id, x, y, 3);
}

int order_patrol(int veh_id, int x, int y) {
    if (!valid_veh(veh_id) || !action_patrol(veh_id, x, y)) {
        return 0;
    }
    Vehs[veh_id].status_icon = 'P';
    run_standing_order(veh_id);
    return 1;
}

int order_continue(int veh_id) {
    if (!valid_veh(veh_id)) {
        return 0;
    }
    uint8_t order = Vehs[veh_id].order;
    if (order >= VehOrderFormerFirst && order <= VehOrderFormerLast) {
        return action(veh_id);
    }
    return run_standing_order(veh_id) > 0;
}

} // namespace engine
} // namespace alpha
