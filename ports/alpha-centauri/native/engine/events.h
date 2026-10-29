/* SPDX-License-Identifier: GPL-3.0-or-later
 * Event/notification sink. Replaces the executable's popup, message and redraw
 * calls (popp, popb, X_pop*, X_pops*, X_dialog, NetMsg_pop, parse_says,
 * parse_num, draw_map, draw_tile, draw_radius) so engine code never draws.
 * Function names and parameters follow Thinker Mod (src/engine.h and
 * src/gui_dialog.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT; see
 * engine/THINKER-LICENSE) so ported calls compile unchanged.
 *
 * The UI installs its own EventSink to render dialogs and the message log; the
 * default HeadlessSink records everything and answers with a fixed choice.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

extern const char* ScriptFile;      // "SCRIPT"
extern const char* PopupScriptFile; // "modmenu"

/* Which original entry point raised a popup. */
enum PopupKind {
    POP_POPP = 0,       // popp(filename, label, flags, pcx, fn)
    POP_POPB = 1,       // popb(label, flags, sound_id, pcx, sprite)
    POP_X_POP = 2,      // X_pop / X_pop_2 / X_pop_6 / X_pop_9
    POP_X_POPS = 3,     // X_pops / X_pops_11 / X_pops_18 / X_dialog
    POP_NETMSG = 4,     // NetMsg_pop: transient message-log notification
};

/* One dialog request. Text substitutions ($STR0.., $NUM0..) are the values
 * set by parse_says/parse_num before the call and are copied here. Pointers
 * are valid only during the callback. */
struct PopupRequest {
    PopupKind kind;
    const char* filename;   // script file such as "SCRIPT"; never null
    const char* label;      // #LABEL inside the script file; never null
    const char* image;      // pcx file name or null
    const Sprite* sprite;   // portrait or null
    int flags;              // original flags/a3 argument
    int sound_id;           // popb sound id, else -1
    int delay;              // NetMsg_pop delay in ms, else 0
    int list_index;         // X_pop_9/X_pops_18 initial selection, else -1
    int faction_id;         // portrait faction for X_dialog, else -1
    const char* strings[10];
    int numbers[10];
};

enum RedrawKind {
    REDRAW_MAP = 0,     // draw_map(flag)
    REDRAW_TILE = 1,    // draw_tile(x, y, flag)
    REDRAW_RADIUS = 2,  // draw_radius(x, y, radius, flag)
};

struct RedrawRequest {
    RedrawKind kind;
    int x;
    int y;
    int radius;
    int flag;
};

/* Structured state-change notifications for UI bookkeeping that the original
 * did inside game logic (BaseWin_check_*, MessageWin list edits, etc.). */
enum EngineEventType {
    EV_VEH_CREATED = 0,     // a = veh_id, b = unit_id, c = faction_id, x/y
    EV_VEH_KILLED = 1,      // a = old veh_id, b = unit_id, c = faction_id, d = home_base_id, x/y
    EV_BASE_CREATED = 2,    // a = base_id, c = faction_id, x/y
    EV_BASE_KILLED = 3,     // a = old base_id, c = faction_id, x/y; ids above a shift down by one
    EV_MAP_CHANGED = 4,     // map arrays reallocated (load, new map)
    EV_GAME_LOADED = 5,     // a = load status
    EV_GAME_SAVED = 6,      // a = save status
    EV_NOT_IMPLEMENTED = 7, // text = function name; raised by package stubs
    EV_MESSAGE = 8,         // free text (debug/log)
};

struct EngineEvent {
    EngineEventType type;
    int a;
    int b;
    int c;
    int d;
    int x;
    int y;
    const char* text;
};

class EventSink {
    public:
    virtual ~EventSink();
    /* Return the chosen option (0 = first/OK), or -1 for cancel. */
    virtual int popup(const PopupRequest& request);
    virtual void redraw(const RedrawRequest& request);
    virtual void notify(const EngineEvent& event);
};

/* Default sink: keeps a bounded text log and answers every popup with
 * default_choice, or with the value set for that label. */
class HeadlessSink : public EventSink {
    public:
    int default_choice = 0;
    size_t max_log = 4096;
    std::map<std::string, int> choices; // label -> answer
    std::vector<std::string> log;
    size_t popups = 0;
    size_t redraws = 0;
    size_t notifications = 0;
    size_t not_implemented = 0;

    int popup(const PopupRequest& request) override;
    void redraw(const RedrawRequest& request) override;
    void notify(const EngineEvent& event) override;
    void clear();
    private:
    void add(const std::string& line);
};

/* Install a sink (null restores the built-in HeadlessSink). Returns previous. */
EventSink* set_event_sink(EventSink* sink);
EventSink* event_sink();
HeadlessSink* headless_sink();

void engine_notify(EngineEventType type, int a = 0, int b = 0, int c = 0, int d = 0,
    int x = -1, int y = -1, const char* text = nullptr);
/* Package stubs call this; it raises EV_NOT_IMPLEMENTED once per call. */
void not_implemented(const char* function_name);

/* ---- Original entry points (Thinker names) ---- */
int pop_wait();
int pop_none();
int popp(const char* filename, const char* label, int a3, const char* pcx_filename, fp_none fn);
int popb(const char* label, int flags, int sound_id, const char* pcx_filename, Sprite* a5);
int X_pop_9(const char* filename, const char* label, int a3, int a4, int a5, fp_none fn);
int X_pops_18(const char* filename, const char* label, int a3, int a4, int a5, Sprite* sprite,
    int a7, int a8, fp_none fn);
int X_pop(const char* label, fp_none fn);
int X_pop_2(const char* filename, const char* label, fp_none fn);
int X_pop_6(const char* label, int a2, fp_none fn);
int X_pops(const char* label, Sprite* a2, fp_none fn);
int X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn);
int X_dialog(const char* label, int faction2);
int X_dialog(const char* filename, const char* label, int faction2);
int NetMsg_pop(NetMessage* This, const char* label, int delay, int a4, const char* filename);
int parse_says(int index, const char* text, int gender, int plural);
int parse_num(int index, int value);
void draw_map(int flag);
void draw_tile(int x, int y, int flag);
void draw_radius(int x, int y, int radius, int flag);

} // namespace engine
} // namespace alpha
