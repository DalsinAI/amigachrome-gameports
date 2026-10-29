/* SPDX-License-Identifier: GPL-3.0-or-later
 * Event sink and headless implementations of the original popup entry points.
 * X_pop/X_pop_2/X_pop_6/X_pops/X_pops_11/X_dialog follow Thinker Mod
 * src/gui_dialog.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af
 * (MIT; see engine/THINKER-LICENSE).
 */
#include "engine.h"

namespace alpha {
namespace engine {

const char* ScriptFile = "SCRIPT";
const char* PopupScriptFile = "modmenu";

static HeadlessSink default_sink;
static EventSink* current_sink = &default_sink;

EventSink::~EventSink() {}
int EventSink::popup(const PopupRequest&) { return 0; }
void EventSink::redraw(const RedrawRequest&) {}
void EventSink::notify(const EngineEvent&) {}

void HeadlessSink::add(const std::string& line) {
    if (log.size() < max_log) {
        log.push_back(line);
    }
}

void HeadlessSink::clear() {
    log.clear();
    popups = redraws = notifications = not_implemented = 0;
}

int HeadlessSink::popup(const PopupRequest& r) {
    popups++;
    std::string line = "popup ";
    line += r.filename;
    line += "#";
    line += r.label;
    for (int i = 0; i < 10; i++) {
        if (r.strings[i] && r.strings[i][0]) {
            char buf[32];
            snprintf(buf, sizeof(buf), " $STR%d=", i);
            line += buf;
            line += r.strings[i];
        }
    }
    add(line);
    auto it = choices.find(r.label);
    return it != choices.end() ? it->second : default_choice;
}

void HeadlessSink::redraw(const RedrawRequest&) {
    redraws++;
}

void HeadlessSink::notify(const EngineEvent& e) {
    notifications++;
    if (e.type == EV_NOT_IMPLEMENTED) {
        not_implemented++;
    }
    char buf[160];
    snprintf(buf, sizeof(buf), "event %d %d %d %d %d (%d,%d) %s", (int)e.type,
        e.a, e.b, e.c, e.d, e.x, e.y, e.text ? e.text : "");
    add(buf);
}

EventSink* set_event_sink(EventSink* sink) {
    EventSink* previous = current_sink;
    current_sink = sink ? sink : &default_sink;
    return previous;
}

EventSink* event_sink() {
    return current_sink;
}

HeadlessSink* headless_sink() {
    return &default_sink;
}

void engine_notify(EngineEventType type, int a, int b, int c, int d, int x, int y, const char* text) {
    EngineEvent e = {type, a, b, c, d, x, y, text};
    current_sink->notify(e);
}

void not_implemented(const char* function_name) {
    debug("not_implemented %s\n", function_name);
    engine_notify(EV_NOT_IMPLEMENTED, 0, 0, 0, 0, -1, -1, function_name);
}

int pop_wait() {
    return 0;
}

int pop_none() {
    return 0;
}

static int raise_popup(PopupKind kind, const char* filename, const char* label,
const char* image, const Sprite* sprite, int flags, int sound_id, int delay,
int list_index, int faction_id) {
    PopupRequest r;
    r.kind = kind;
    r.filename = filename ? filename : ScriptFile;
    r.label = label ? label : "";
    r.image = image;
    r.sprite = sprite;
    r.flags = flags;
    r.sound_id = sound_id;
    r.delay = delay;
    r.list_index = list_index;
    r.faction_id = faction_id;
    for (int i = 0; i < 10; i++) {
        r.strings[i] = ParseStrBuffer ? ParseStrBuffer[i].str : "";
        r.numbers[i] = ParseNumTable ? ParseNumTable[i] : 0;
    }
    return current_sink->popup(r);
}

int popp(const char* filename, const char* label, int a3, const char* pcx_filename, fp_none UNUSED(fn)) {
    return raise_popup(POP_POPP, filename, label, pcx_filename, nullptr, a3, -1, 0, -1, -1);
}

int popb(const char* label, int flags, int sound_id, const char* pcx_filename, Sprite* a5) {
    return raise_popup(POP_POPB, ScriptFile, label, pcx_filename, a5, flags, sound_id, 0, -1, -1);
}

int X_pop_9(const char* filename, const char* label, int a3, int UNUSED(a4), int a5, fp_none UNUSED(fn)) {
    return raise_popup(POP_X_POP, filename, label, nullptr, nullptr, a5, -1, 0, a3, -1);
}

static int sprite_faction(const Sprite* sprite) {
    for (int i = 0; i < MaxPlayerNum && sprite; i++) {
        if (FactionPortraits[i] == sprite) {
            return i;
        }
    }
    return -1;
}

int X_pops_18(const char* filename, const char* label, int a3, int UNUSED(a4), int a5, Sprite* sprite,
int UNUSED(a7), int UNUSED(a8), fp_none UNUSED(fn)) {
    return raise_popup(POP_X_POPS, filename, label, nullptr, sprite, a5, -1, 0, a3, sprite_faction(sprite));
}

int X_pop(const char* label, fp_none fn) {
    if (!conf.warn_on_former_replace && !strcmp(label, "MIMIMI")) {
        return 1;
    }
    return X_pop_9(ScriptFile, label, -1, 0, 0, fn);
}

int X_pop_2(const char* filename, const char* label, fp_none fn) {
    return X_pop_9(filename, label, -1, 0, 0, fn);
}

int X_pop_6(const char* label, int a2, fp_none fn) {
    return X_pop_9(ScriptFile, label, -1, 0, a2, fn);
}

int X_pops(const char* label, Sprite* a2, fp_none fn) {
    return X_pops_18(ScriptFile, label, -1, 0, 0, a2, 1, 1, fn);
}

int X_pops_11(const char* label, int a2, Sprite* a3, fp_none fn) {
    return X_pops_18(ScriptFile, label, -1, 0, a2, a3, 1, 1, fn);
}

/* PopDialogUnk100000 = 0x100000 (gui_dialog.h) */
int X_dialog(const char* label, int faction2) {
    return raise_popup(POP_X_POPS, ScriptFile, label, nullptr,
        faction2 >= 0 && faction2 < MaxPlayerNum ? FactionPortraits[faction2] : nullptr,
        0x100000, -1, 0, -1, faction2);
}

int X_dialog(const char* filename, const char* label, int faction2) {
    return raise_popup(POP_X_POPS, filename, label, nullptr,
        faction2 >= 0 && faction2 < MaxPlayerNum ? FactionPortraits[faction2] : nullptr,
        0x100000, -1, 0, -1, faction2);
}

int NetMsg_pop(NetMessage* UNUSED(This), const char* label, int delay, int a4, const char* filename) {
    return raise_popup(POP_NETMSG, filename ? filename : ScriptFile, label, nullptr, nullptr,
        a4, -1, delay, -1, -1);
}

int parse_says(int index, const char* text, int gender, int plural) {
    if (index < 0 || index >= 10 || !ParseStrBuffer) {
        return 0;
    }
    snprintf(ParseStrBuffer[index].str, sizeof(ParseStrBuffer[index].str), "%s", text ? text : "");
    ParseStrGender[index] = gender;
    ParseStrPlurality[index] = plural;
    return 0;
}

int parse_num(int index, int value) {
    if (index < 0 || index >= 10 || !ParseNumTable) {
        return 0;
    }
    ParseNumTable[index] = value;
    return 0;
}

void draw_map(int flag) {
    RedrawRequest r = {REDRAW_MAP, -1, -1, 0, flag};
    current_sink->redraw(r);
}

void draw_tile(int x, int y, int flag) {
    RedrawRequest r = {REDRAW_TILE, x, y, 0, flag};
    current_sink->redraw(r);
}

void draw_radius(int x, int y, int radius, int flag) {
    RedrawRequest r = {REDRAW_RADIUS, x, y, radius, flag};
    current_sink->redraw(r);
}

} // namespace engine
} // namespace alpha
