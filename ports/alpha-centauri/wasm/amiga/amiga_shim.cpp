/* SPDX-License-Identifier: GPL-3.0-or-later
 * Browser stand-in for the AmigaOS calls of the native frontend; see
 * alpha_amiga_shim.h. Windows are RGB frames stacked in open order; the top
 * one is copied to the page canvas (shell.js: alphaPresent). Input arrives
 * as IntuiMessages translated by shell.js from DOM events. WaitPort and the
 * file requester sleep with ASYNCIFY, so the native event loops run as they
 * are while the browser keeps painting and delivering input.
 */
#include "alpha_amiga_shim.h"
#include <emscripten.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* JS side (shell.js installs Module.alpha*). */
EM_JS(void, alpha_js_present, (const uint8_t* rgb, int width, int height, const char* title), {
 if (Module.alphaPresent) Module.alphaPresent(HEAPU8.subarray(rgb, rgb + width * height * 3), width, height, UTF8ToString(title));
});
EM_JS(int, alpha_js_next_event, (int* out), {
 const q = Module.alphaEvents;
 if (!q || !q.length) return 0;
 const e = q.shift();
 HEAP32[(out >> 2) + 0] = e.cls; HEAP32[(out >> 2) + 1] = e.code;
 HEAP32[(out >> 2) + 2] = e.x; HEAP32[(out >> 2) + 3] = e.y;
 return 1;
});
EM_ASYNC_JS(int, alpha_js_wait_event, (int timeout_ms), {
 if (Module.alphaWaitEvent) return await Module.alphaWaitEvent(timeout_ms);
 await new Promise(r => setTimeout(r, timeout_ms));
 return 0;
});
EM_ASYNC_JS(int, alpha_js_ask_file, (const char* title, char* out, int out_size), {
 if (!Module.alphaAskFile) return 0;
 const name = await Module.alphaAskFile(UTF8ToString(title));
 if (!name) return 0;
 const n = stringToUTF8(name, out, out_size);
 return n > 0 && n < out_size - 1 ? 1 : 0;
});

extern "C" void alpha_wasm_tick();
extern "C" void alpha_wasm_waiting(int now_waiting);
extern "C" void alpha_wasm_yield_point();

namespace {
const int BORDER_LEFT = 4, BORDER_TOP = 20, BORDER_RIGHT = 4, BORDER_BOTTOM = 2;
const int CLOSE_GADGET = 20;
struct Lib { struct Library lib; };
Lib libraries[8];
struct Window* top = nullptr;
bool dirty = false;
uint8_t pens[256][3] = {{149, 149, 149}, {0, 0, 0}, {255, 255, 255}, {59, 103, 162}};
bool pen_used[256] = {true, true, true, true};
struct ColorMap colormap;
struct Screen screen = {{&colormap}};
struct Queued { struct IntuiMessage m; struct Queued* next; };
Queued* head = nullptr;
Queued* tail = nullptr;

void present() {
 if (!top) return;
 alpha_js_present(top->alpha_rgb, top->Width, top->Height, top->alpha_title);
 dirty = false;
}
void touch(struct RastPort* rp) { if (rp && rp->window == top) dirty = true; }
bool clip(struct Window* w, int& x, int& y, int& width, int& height, int* sx = nullptr, int* sy = nullptr) {
 if (x < 0) { width += x; if (sx) *sx -= x; x = 0; }
 if (y < 0) { height += y; if (sy) *sy -= y; y = 0; }
 if (x + width > w->Width) width = w->Width - x;
 if (y + height > w->Height) height = w->Height - y;
 return width > 0 && height > 0;
}
void fill(struct Window* w, int x, int y, int width, int height, const uint8_t* rgb) {
 if (!clip(w, x, y, width, height)) return;
 for (int row = 0; row < height; ++row) {
  uint8_t* p = w->alpha_rgb + ((size_t)(y + row) * w->Width + x) * 3;
  for (int col = 0; col < width; ++col, p += 3) { p[0] = rgb[0]; p[1] = rgb[1]; p[2] = rgb[2]; }
 }
}
void decorate(struct Window* w) {
 static const uint8_t back[3] = {149, 149, 149}, bar[3] = {59, 103, 162}, black[3] = {0, 0, 0}, white[3] = {255, 255, 255};
 fill(w, 0, 0, w->Width, w->Height, back);
 fill(w, 0, 0, w->Width, BORDER_TOP, bar);
 fill(w, 3, 3, CLOSE_GADGET - 6, BORDER_TOP - 6, black);
 fill(w, 6, 6, CLOSE_GADGET - 12, BORDER_TOP - 12, white);
}
/* Pull translated DOM input into this queue; drop what the window did not ask for. */
void pump() {
 int e[4];
 while (alpha_js_next_event(e)) {
  if (!top) continue;
  ULONG cls = (ULONG)e[0];
  UWORD code = (UWORD)e[1];
  if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN && e[3] < BORDER_TOP && e[2] < CLOSE_GADGET) cls = IDCMP_CLOSEWINDOW;
  if (!(top->IDCMPFlags & cls)) continue;
  auto* q = (Queued*)calloc(1, sizeof(Queued));
  if (!q) return;
  q->m.Class = cls; q->m.Code = code; q->m.MouseX = (WORD)e[2]; q->m.MouseY = (WORD)e[3];
  q->m.IDCMPWindow = top;
  time_t now = time(nullptr); q->m.Seconds = (ULONG)now;
  if (tail) tail->next = q; else head = q;
  tail = q;
 }
}
void drop_queue() {
 while (head) { Queued* n = head->next; free(head); head = n; }
 tail = nullptr;
}
}

extern "C" {
struct Library* OpenLibrary(STRPTR name, ULONG) {
 static const char* const known[] = {"intuition.library", "graphics.library", "cybergraphics.library", "asl.library", "dos.library", "exec.library"};
 for (unsigned i = 0; i < sizeof(known) / sizeof(known[0]); ++i)
  if (name && !strcmp(name, known[i])) return &libraries[i].lib;
 return nullptr;
}
void CloseLibrary(struct Library*) {}

struct Message* GetMsg(struct MsgPort*) {
 pump();
 alpha_wasm_tick();
 if (dirty) present();
 if (!head) return nullptr;
 Queued* q = head;
 head = q->next;
 if (!head) tail = nullptr;
 q->next = nullptr;
 return &q->m.ExecMessage;
}
void ReplyMsg(struct Message* message) { free(message); }
struct Message* WaitPort(struct MsgPort*) {
 alpha_wasm_waiting(1);
 for (;;) {
  pump();
  if (head) { alpha_wasm_waiting(0); return &head->m.ExecMessage; }
  if (dirty) present();
  alpha_wasm_tick();
  alpha_js_wait_event(40);
 }
}

struct Window* OpenWindowTags(struct NewWindow*, ULONG tag, ...) {
 int width = 640, height = 480, inner_width = -1, inner_height = -1;
 ULONG idcmp = 0;
 const char* title = "";
 va_list ap;
 va_start(ap, tag);
 for (ULONG t = tag; t != TAG_DONE; t = va_arg(ap, ULONG)) {
  ULONG v = va_arg(ap, ULONG);
  switch (t) {
  case WA_Width: width = (int)v; break;
  case WA_Height: height = (int)v; break;
  case WA_InnerWidth: inner_width = (int)v; break;
  case WA_InnerHeight: inner_height = (int)v; break;
  case WA_IDCMP: idcmp = v; break;
  case WA_Title: title = v ? (const char*)(uintptr_t)v : ""; break;
  default: break;
  }
 }
 va_end(ap);
 if (inner_width > 0) width = inner_width + BORDER_LEFT + BORDER_RIGHT;
 if (inner_height > 0) height = inner_height + BORDER_TOP + BORDER_BOTTOM;
 if (width <= 0 || height <= 0 || width > 4096 || height > 4096) return nullptr;
 auto* w = (struct Window*)calloc(1, sizeof(struct Window));
 auto* rp = (struct RastPort*)calloc(1, sizeof(struct RastPort));
 auto* port = (struct MsgPort*)calloc(1, sizeof(struct MsgPort));
 auto* rgb = (uint8_t*)malloc((size_t)width * height * 3);
 if (!w || !rp || !port || !rgb) { free(w); free(rp); free(port); free(rgb); return nullptr; }
 w->Width = (WORD)width; w->Height = (WORD)height;
 w->BorderLeft = BORDER_LEFT; w->BorderTop = BORDER_TOP; w->BorderRight = BORDER_RIGHT; w->BorderBottom = BORDER_BOTTOM;
 w->RPort = rp; w->UserPort = port; w->WScreen = &screen; w->IDCMPFlags = idcmp; w->alpha_rgb = rgb;
 snprintf(w->alpha_title, sizeof(w->alpha_title), "%s", title);
 rp->window = w; rp->apen = 1; rp->bpen = 0; port->owner = w;
 decorate(w);
 drop_queue();
 w->alpha_below = top;
 top = w;
 dirty = true;
 return w;
}
void CloseWindow(struct Window* window) {
 if (!window) return;
 if (window == top) top = window->alpha_below;
 else for (struct Window* w = top; w; w = w->alpha_below) if (w->alpha_below == window) { w->alpha_below = window->alpha_below; break; }
 drop_queue();
 free(window->alpha_rgb); free(window->RPort); free(window->UserPort); free(window);
 dirty = top != nullptr;
 if (top) present();
}
void ActivateWindow(struct Window* window) { if (window && window == top) present(); }
void BeginRefresh(struct Window*) {}
void EndRefresh(struct Window*, BOOL) {}

void SetAPen(struct RastPort* rp, ULONG pen) { if (rp) rp->apen = pen & 255; }
void SetBPen(struct RastPort* rp, ULONG pen) { if (rp) rp->bpen = pen & 255; }
void SetDrMd(struct RastPort* rp, ULONG mode) { if (rp) rp->drawmode = mode; }
void Move(struct RastPort* rp, LONG x, LONG y) { if (rp) { rp->cp_x = x; rp->cp_y = y; } }
void Draw(struct RastPort* rp, LONG x, LONG y) {
 if (!rp || !rp->window) return;
 struct Window* w = rp->window;
 const uint8_t* c = pens[rp->apen & 255];
 LONG x0 = rp->cp_x, y0 = rp->cp_y;
 const LONG dx = labs(x - x0), dy = -labs(y - y0), sx = x0 < x ? 1 : -1, sy = y0 < y ? 1 : -1;
 LONG err = dx + dy;
 for (;;) {
  if (x0 >= 0 && y0 >= 0 && x0 < w->Width && y0 < w->Height) memcpy(w->alpha_rgb + ((size_t)y0 * w->Width + x0) * 3, c, 3);
  if (x0 == x && y0 == y) break;
  const LONG e2 = 2 * err;
  if (e2 >= dy) { err += dy; x0 += sx; }
  if (e2 <= dx) { err += dx; y0 += sy; }
 }
 rp->cp_x = x; rp->cp_y = y;
 touch(rp);
}
LONG ObtainBestPen(struct ColorMap*, ULONG r, ULONG g, ULONG b, ULONG, ...) {
 for (int i = 4; i < 256; ++i) if (!pen_used[i]) {
  pen_used[i] = true;
  pens[i][0] = (uint8_t)(r >> 24); pens[i][1] = (uint8_t)(g >> 24); pens[i][2] = (uint8_t)(b >> 24);
  return i;
 }
 return -1;
}
void ReleasePen(struct ColorMap*, ULONG pen) { if (pen >= 4 && pen < 256) pen_used[pen] = false; }

ULONG WritePixelArray(APTR src, UWORD srcx, UWORD srcy, UWORD srcmod, struct RastPort* rp, UWORD dx, UWORD dy, UWORD width, UWORD height, UBYTE) {
 alpha_wasm_yield_point(); /* long redraws (many text runs) also let the page breathe */
 if (!src || !rp || !rp->window) return 0;
 struct Window* w = rp->window;
 int x = dx, y = dy, wd = width, ht = height, sx = srcx, sy = srcy;
 if (!clip(w, x, y, wd, ht, &sx, &sy)) return 0;
 for (int row = 0; row < ht; ++row)
  memcpy(w->alpha_rgb + ((size_t)(y + row) * w->Width + x) * 3, (const uint8_t*)src + (size_t)(sy + row) * srcmod + (size_t)sx * 3, (size_t)wd * 3);
 touch(rp);
 return (ULONG)(wd * ht);
}
ULONG ReadPixelArray(APTR dst, UWORD dstx, UWORD dsty, UWORD dstmod, struct RastPort* rp, UWORD sx, UWORD sy, UWORD width, UWORD height, UBYTE) {
 if (!dst || !rp || !rp->window) return 0;
 struct Window* w = rp->window;
 int x = sx, y = sy, wd = width, ht = height, tx = dstx, ty = dsty;
 if (!clip(w, x, y, wd, ht, &tx, &ty)) return 0;
 for (int row = 0; row < ht; ++row)
  memcpy((uint8_t*)dst + (size_t)(ty + row) * dstmod + (size_t)tx * 3, w->alpha_rgb + ((size_t)(y + row) * w->Width + x) * 3, (size_t)wd * 3);
 return (ULONG)(wd * ht);
}
ULONG FillPixelArray(struct RastPort* rp, UWORD x, UWORD y, UWORD width, UWORD height, ULONG argb) {
 if (!rp || !rp->window) return 0;
 const uint8_t rgb[3] = {(uint8_t)(argb >> 16), (uint8_t)(argb >> 8), (uint8_t)argb};
 fill(rp->window, x, y, width, height, rgb);
 touch(rp);
 return (ULONG)width * height;
}

struct ShimRequester { struct FileRequester fr; char title[160]; char file[512]; char drawer[8]; };
APTR AllocAslRequestTags(ULONG type, ULONG tag, ...) {
 if (type != ASL_FileRequest) return nullptr;
 auto* r = (ShimRequester*)calloc(1, sizeof(ShimRequester));
 if (!r) return nullptr;
 snprintf(r->title, sizeof(r->title), "Open a file");
 va_list ap;
 va_start(ap, tag);
 for (ULONG t = tag; t != TAG_DONE; t = va_arg(ap, ULONG)) {
  ULONG v = va_arg(ap, ULONG);
  if (t == ASLFR_TitleText && v) snprintf(r->title, sizeof(r->title), "%s", (const char*)(uintptr_t)v);
 }
 va_end(ap);
 r->fr.fr_File = r->file; r->fr.fr_Drawer = r->drawer;
 return r;
}
BOOL AslRequestTags(APTR request, ULONG, ...) {
 auto* r = (ShimRequester*)request;
 if (!r) return FALSE;
 present();
 r->file[0] = 0; r->drawer[0] = 0;
 alpha_wasm_waiting(1);
 const int ok = alpha_js_ask_file(r->title, r->file, (int)sizeof(r->file));
 alpha_wasm_waiting(0);
 drop_queue();
 return ok && r->file[0] ? TRUE : FALSE;
}
void FreeAslRequest(APTR request) { free(request); }

struct DateStamp* DateStamp(struct DateStamp* stamp) {
 const time_t now = time(nullptr);
 stamp->ds_Days = (LONG)(now / 86400 - 2922); /* days since 1 Jan 1978 */
 stamp->ds_Minute = (LONG)(now % 86400 / 60);
 stamp->ds_Tick = (LONG)(now % 60 * 50);
 return stamp;
}
BPTR CreateDir(STRPTR name) { return name && mkdir(name, 0777) == 0 ? 1 : 0; }
void UnLock(BPTR) {}
}
