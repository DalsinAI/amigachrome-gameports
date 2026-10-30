/* SPDX-License-Identifier: GPL-3.0-or-later
 * Browser-only runtime pieces around the unchanged native code:
 *
 * 1. Cooperative "threads" for SDL_CreateThread in native/audio_service.cpp
 *    (see compat/sdl_coop_thread.h): the music feeder runs one iteration per
 *    tick, so music streams without pthreads.
 * 2. Yield points inside the engine. The link wraps (-Wl,--wrap) engine
 *    functions that run all through an AI turn: the Thinker debug() logger
 *    (unit moves, base upkeep, planning), Path_find and the AI tile search
 *    step ai::TileSearch::get_next (the two hottest AI paths in a node CPU
 *    profile of the 20-turn play test), iterate_tiles (the AI's area scans),
 *    action and order_veh (each unit's move); found with Chromium CPU
 *    profiles of a throttled End Turn. At most every 25 ms the
 *    browser gets control back (ASYNCIFY): the page keeps painting and taking
 *    input, music keeps streaming, and an "AI turn in progress" indicator
 *    shows once the program has been busy for a moment. Each wrapper then
 *    does exactly what the native function does (debug() is re-implemented:
 *    print to debug_log when one is open; the others call the original).
 * 3. Audio call records: the native audio entry points used by the world
 *    view are wrapped to note each call (Module.alphaAudioEvents and stdout)
 *    and then run unchanged.
 */
#include "audio_events.h"
#include "audio_service.h"
#include "engine/engine.h" /* MapTile, for the iterate_tiles wrapper */
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

EM_ASYNC_JS(void, alpha_js_yield, (double busy_ms, double gap_ms, int from, int to), {
 if (Module.alphaBusy) Module.alphaBusy(busy_ms, gap_ms, from, to);
 await new Promise((resolve) => { const c = new MessageChannel(); c.port1.onmessage = () => resolve(); c.port2.postMessage(0); });
});
EM_JS(void, alpha_js_idle, (), { if (Module.alphaIdle) Module.alphaIdle(); });
EM_JS(void, alpha_js_audio_note, (const char* what), {
 const text = UTF8ToString(what);
 (Module.alphaAudioEvents = Module.alphaAudioEvents || []).push({ t: performance.now(), what: text });
});

namespace {
struct Coop { SDL_ThreadFunction fn; void* data; bool live; int checks; };
Coop coops[4];
Coop* active = nullptr;
double last_tick = 0, last_yield = 0, busy_since = 0;
bool waiting = false;
int last_site = 0; /* which yield point gave control back last (for the stats) */
}

extern "C" {
SDL_Thread* alpha_coop_create_thread(SDL_ThreadFunction fn, const char*, void* data) {
 for (auto& c : coops) if (!c.live) { c.fn = fn; c.data = data; c.live = true; c.checks = 0; return (SDL_Thread*)(&c); }
 return nullptr;
}
void alpha_coop_wait_thread(SDL_Thread* thread, int* status) {
 // The thread function returns once its running flag is clear: run it once
 // more so it sees the flag, as a joined native thread would.
 auto* c = (Coop*)thread;
 if (c && c->live && !active) { active = c; c->checks = 0; int r = c->fn(c->data); active = nullptr; if (status) *status = r; }
 else if (status) *status = 0;
 if (c) c->live = false;
}
int alpha_coop_atomic_get(SDL_atomic_t* value) {
 // Inside a tick: the loop condition is true once (one iteration), then false.
 if (active && active->checks++) return 0;
 return SDL_AtomicGet(value);
}
void alpha_coop_delay(Uint32 ms) { if (!active) SDL_Delay(ms); }

/* Called by the Amiga stand-in while it waits for input, and by the yield point. */
void alpha_wasm_tick() {
 const double now = emscripten_get_now();
 if (active || now - last_tick < 20) return;
 last_tick = now;
 for (auto& c : coops) if (c.live) { active = &c; c.checks = 0; c.fn(c.data); active = nullptr; }
}
void alpha_wasm_waiting(int now_waiting) {
 if (now_waiting && !waiting) alpha_js_idle();
 if (!now_waiting && waiting) busy_since = emscripten_get_now();
 waiting = now_waiting != 0;
}
/* site: 1 debug, 2 Path_find, 3 tile search, 4 iterate_tiles, 5 action,
 * 6 order_veh, 7 window drawing. The page keeps the longest stretch between
 * two yields and the sites at both ends (Module.alphaBusyStats). */
void alpha_wasm_yield_site(int site) {
 if (waiting || active) return;
 const double now = emscripten_get_now();
 if (now - last_yield < 25) return;
 alpha_wasm_tick();
 alpha_js_yield(now - busy_since, last_yield > busy_since ? now - last_yield : now - busy_since, last_site, site);
 last_site = site;
 last_yield = emscripten_get_now();
}
void alpha_wasm_yield_point() { alpha_wasm_yield_site(7); }
}

/* 2. alpha::engine::debug (Thinker logger), wrapped by the link. */
namespace alpha { namespace engine { extern FILE* debug_log; } }
extern "C" void __wrap__ZN5alpha6engine5debugEPKcz(const char* fmt, ...) {
 alpha_wasm_yield_site(1);
 if (alpha::engine::debug_log) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(alpha::engine::debug_log, fmt, ap);
  va_end(ap);
 }
}

struct MAP;
extern "C" int __real__ZN5alpha6engine9Path_findEiiiiiiii(int, int, int, int, int, int, int, int);
extern "C" int __wrap__ZN5alpha6engine9Path_findEiiiiiiii(int x1, int y1, int x2, int y2, int unit_id, int faction_id, int flags, int unk) {
 alpha_wasm_yield_site(2);
 return __real__ZN5alpha6engine9Path_findEiiiiiiii(x1, y1, x2, y2, unit_id, faction_id, flags, unk);
}
extern "C" MAP* __real__ZN5alpha6engine2ai10TileSearch8get_nextEv(void* self);
extern "C" MAP* __wrap__ZN5alpha6engine2ai10TileSearch8get_nextEv(void* self) {
 static unsigned calls = 0;
 if (!(++calls & 255)) alpha_wasm_yield_site(3); /* called per tile: check the clock every 256 steps */
 return __real__ZN5alpha6engine2ai10TileSearch8get_nextEv(self);
}
/* The AI's area scans (ai::move_upkeep and friends) and each unit's action
 * and orders: the long stretches left between the yield points above. */
namespace alpha { namespace engine {
std::vector<MapTile> real_iterate_tiles(int, int, size_t, size_t) __asm__("__real__ZN5alpha6engine13iterate_tilesEiimm");
std::vector<MapTile> wrap_iterate_tiles(int x, int y, size_t start_index, size_t end_index) __asm__("__wrap__ZN5alpha6engine13iterate_tilesEiimm");
std::vector<MapTile> wrap_iterate_tiles(int x, int y, size_t start_index, size_t end_index) {
 static unsigned calls = 0;
 if (!(++calls & 15)) alpha_wasm_yield_site(4);
 return real_iterate_tiles(x, y, start_index, end_index);
}
}}
extern "C" int __real__ZN5alpha6engine6actionEi(int);
extern "C" int __wrap__ZN5alpha6engine6actionEi(int veh_id) {
 alpha_wasm_yield_site(5);
 return __real__ZN5alpha6engine6actionEi(veh_id);
}
extern "C" int __real__ZN5alpha6engine9order_vehEiii(int, int, int);
extern "C" int __wrap__ZN5alpha6engine9order_vehEiii(int veh_id, int offset, int flag) {
 alpha_wasm_yield_site(6);
 return __real__ZN5alpha6engine9order_vehEiii(veh_id, offset, flag);
}

/* 3. Audio call records. */
extern "C" {
bool __real__ZN5alpha10audio_openEPKcS1_(const char*, const char*);
void __real__ZN5alpha11audio_eventENS_10AudioEventE(alpha::AudioEvent);
bool __real__ZN5alpha17audio_world_musicEPKc(const char*);
bool __real__ZN5alpha20audio_faction_speechEPKcb(const char*, bool);
bool __real__ZN5alpha21audio_tech_discoveredEib(int, bool);
bool __real__ZN5alpha23audio_facility_completeEib(int, bool);
}
static void note(const char* fmt, ...) {
 char line[200];
 va_list ap;
 va_start(ap, fmt);
 vsnprintf(line, sizeof(line), fmt, ap);
 va_end(ap);
 alpha_js_audio_note(line);
 printf("WASM AUDIO %s\n", line);
 fflush(stdout);
}
extern "C" bool __wrap__ZN5alpha10audio_openEPKcS1_(const char* dir, const char* settings) {
 const bool ok = __real__ZN5alpha10audio_openEPKcS1_(dir, settings);
 note("open device=%d settings=%s status=%s", ok, settings ? settings : "-", alpha::audio_status().last_error);
 return ok;
}
extern "C" void __wrap__ZN5alpha11audio_eventENS_10AudioEventE(alpha::AudioEvent e) {
 __real__ZN5alpha11audio_eventENS_10AudioEventE(e);
 const char* file = alpha::audio_event_file(alpha::audio_settings(), e);
 note("event %s file=%s", alpha::audio_event_info(e).key, file ? file : "-");
}
extern "C" bool __wrap__ZN5alpha17audio_world_musicEPKc(const char* stem) {
 const bool ok = __real__ZN5alpha17audio_world_musicEPKc(stem);
 note("music faction=%s playing=%d set=%s", stem ? stem : "-", ok, alpha::faction_music_set(stem ? stem : ""));
 return ok;
}
extern "C" bool __wrap__ZN5alpha20audio_faction_speechEPKcb(const char* stem, bool expansion) {
 const bool ok = __real__ZN5alpha20audio_faction_speechEPKcb(stem, expansion);
 note("voice faction=%s started=%d", stem ? stem : "-", ok);
 return ok;
}
extern "C" bool __wrap__ZN5alpha21audio_tech_discoveredEib(int id, bool human) {
 const bool ok = __real__ZN5alpha21audio_tech_discoveredEib(id, human);
 note("voice tech=%d started=%d", id, ok);
 return ok;
}
extern "C" bool __wrap__ZN5alpha23audio_facility_completeEib(int id, bool human) {
 const bool ok = __real__ZN5alpha23audio_facility_completeEib(id, human);
 note("voice facility=%d started=%d", id, ok);
 return ok;
}
