/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP3 private header: presentation-only calls of the original executable that
 * ported Thinker Mod code (dabe62f2, MIT; see engine/THINKER-LICENSE) makes
 * from game logic. The engine never draws, plays sounds or waits, so these
 * keep their Thinker names but only forward redraw requests to the event
 * sink or do nothing. Internal linkage: include only from WP3 sources, after
 * engine.h and veh_action.h.
 */
#pragma once

namespace alpha {
namespace engine {
namespace wp3_ui {

/* Original: Console::focus scrolls the map; the UI follows EV_VEH_* and redraws. */
static inline void Console_focus(Console*, int x, int y, int) { draw_tile(x, y, 2); }
static inline void Console_cursor_next(Console*, int, int) {}
static inline void Console_update_data(Console*, int) {}
static inline void BaseWin_check_veh(void*, int) {}
static inline void BaseWin_check_base(void*, int) {}
static inline void BaseWin_check_loc(void*, int, int, int) {}
static inline void GraphicWin_redraw(void*) {}
static inline void FX_play(void*, int) {}
static inline void FX_fade(void*, int) {}
static inline void FX_stop(void*, int) {}
static inline void FX_play_loc(void*, int, int, int) {}
static inline void FX_engine_pitch(void*, int) {}
static inline void wave_it(int) {}
static inline void ambience(int) {}
static inline void clock_wait(int) {}
static inline void interlude(int, int, int, int) {}
static inline bool shift_key_down() { return false; }
static inline bool ctrl_key_down() { return false; }
/* Original: tutorial popups (TutWin) are not part of the engine. */
static inline int tut_check(int) { return 0; }
static inline void TutWin_tut_map(void*, const char*, int, int, int, void*, int, int, int) {}
/* Original: veh_scoot animates a move; the UI animates from state changes. */
static inline void veh_scoot(int, int, int, int, int) {}
static inline void draw_tiles(int x, int y, int flag) { draw_radius(x, y, 1, flag); }
static inline void draw_tile_fixup(int x, int y, int radius, int flag) { draw_radius(x, y, radius + 1, flag); }

static void* const BaseWin = nullptr;
static void* const TutWin = nullptr;
static void* const Sounds = nullptr;
static void* const ImgMapFarm = nullptr;
static void* const ImgMapSoilEnr = nullptr;
static void* const ImgMapMine = nullptr;
static void* const ImgMapSolar = nullptr;
static void* const InfoWin = nullptr;

/* Original: network multiplayer. MultiplayerActive is never set by this
 * engine, so these calls are unreachable; they stay so the ported branches
 * compile unchanged. */
static void* const NetState = nullptr;
inline net_int_t* wp3_net_veh_slot = nullptr;
static net_int_t** const dword_93E908 = &wp3_net_veh_slot;
static inline void synch_veh(int) {}
static inline void NetDaemon_await_synch(void*) {}
static inline void NetDaemon_unlock_veh(void*) {}
static inline void NetDaemon_await_exec(void*, int) {}
static inline int NetDaemon_order_veh(void*, int, int, int) { return 0; }
static inline int NetDaemon_maybe_lock(void*, net_int_t*, int, int, int, int, int) { return 1; }
static inline int NetDaemon_lock_veh(void*, net_int_t*, int, int, int, int) { return 1; }
static inline void message_data(int, int, int, int, int, int) {}
static inline void message_veh(int, int, int, int) {}

/* Original: Popup/Dialogs/BasePop menus read their item texts from
 * script.txt. Here a menu is one popp() request on the event sink: the label
 * names the menu, the answer is the index of the chosen item, and
 * strings[9] of the PopupRequest lists the item values in order
 * ("0,1,2"), so the UI can show the matching script.txt lines. */
struct Popup {
    char filename[32];
    char label[64];
    int sprite; // Original: Sprite, never loaded here
    void* sprite_icon;
    std::vector<int> dialogs;
};
static inline void Popup_ctor(Popup* p) { p->dialogs.clear(); p->sprite_icon = nullptr; }
static inline void Popup_dtor(Popup* p) { p->dialogs.clear(); }
static inline void Popup_start(Popup* p, const char* filename, const char* label, int, int, int, int) {
    snprintf(p->filename, sizeof(p->filename), "%s", filename);
    snprintf(p->label, sizeof(p->label), "%s", label);
    p->dialogs.clear();
}
static inline void Dialogs_item(std::vector<int>* dialogs, const char*, int value) {
    dialogs->push_back(value);
}
/* Original: X_text_open opens a script.txt section for the popup item texts.
 * Fix: the rules loader's text_open is used when the file is there, but the
 * popup always proceeds (the item values, not their texts, decide the
 * choice); text_get then yields NULL and parse_string leaves the text empty. */
static inline int X_text_open(const char* filename, const char* label) {
    text_open(filename, label);
    return 0;
}
static inline int Buffer_get_pcx_dimensions(const char*, int*, int*) { return 1; }
static inline int Sprite_init(void*, const char*, int, int) { return 1; }
static inline int BasePop_exec_3(Popup* p, int, int) {
    if (p->dialogs.empty()) {
        return 0;
    }
    std::string items;
    for (size_t i = 0; i < p->dialogs.size(); i++) {
        items += (i ? "," : "") + std::to_string(p->dialogs[i]);
    }
    parse_says(9, items.c_str(), -1, -1);
    int choice = popp(p->filename, p->label, 0, nullptr, nullptr);
    if (choice < 0) {
        return 0; // cancel: same as the first item (Original: Escape returned 0)
    }
    return p->dialogs[std::min((size_t)choice, p->dialogs.size() - 1)];
}
template <class F>
struct CleanupHandler {
    F fn;
    ~CleanupHandler() { fn(); }
};
template <class F>
static inline CleanupHandler<F> cleanup_handler(F fn) { return CleanupHandler<F>{fn}; }

/* Original: Console field 0x23BF0 (quick-move state of the last move); the
 * engine Console record does not keep it, so it lives here. */
inline int MapWin_field_23BF0 = 0;

/* terranx abil_index: bit number of an ability flag (index into Ability). */
static inline int abil_index(int ability_flag) {
    for (int i = 0; i < 32; i++) {
        if (ability_flag & (1 << i)) {
            return i;
        }
    }
    return 0;
}
static inline void InfoWin_on_redraw(void*) {}

} // namespace wp3_ui
} // namespace engine
} // namespace alpha
