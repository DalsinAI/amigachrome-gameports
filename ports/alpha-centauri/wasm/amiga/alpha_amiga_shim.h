/* SPDX-License-Identifier: GPL-3.0-or-later
 * Browser stand-in for the small part of AmigaOS exec, intuition, graphics,
 * cybergraphics, asl and dos that the native Alpha Centauri frontend uses
 * (original_frontend.cpp, original_map_window.cpp, original_ui_text.cpp,
 * technology_browser_window.cpp, startup_movie_player.cpp).
 *
 * Only the WebAssembly build includes this, through the include directory
 * wasm/amiga/include whose <exec/types.h>, <proto/intuition.h>, ... headers
 * forward here. The native sources compile unchanged. Each window owns an RGB
 * frame; the topmost one is shown on the page canvas. WaitPort sleeps through
 * emscripten's ASYNCIFY so the browser keeps running while the native event
 * loops wait. This is new code: no AmigaOS or SDK header text is copied, and
 * only the names, fields and constants the frontend uses are declared.
 */
#ifndef ALPHA_AMIGA_SHIM_H
#define ALPHA_AMIGA_SHIM_H
#include <stdint.h>
#include <stddef.h>

typedef unsigned long ULONG;
typedef long LONG;
typedef unsigned short UWORD;
typedef short WORD;
typedef unsigned char UBYTE;
typedef signed char BYTE;
typedef short BOOL;
typedef char* STRPTR;
typedef void* APTR;
typedef long BPTR;
typedef ULONG Tag;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define TAG_DONE 0UL

struct Library { int alpha_shim_id; };
struct Message { struct Message* next; };
struct MsgPort { struct Window* owner; };
struct ColorMap { int alpha_shim_pens; };
struct ViewPort { struct ColorMap* ColorMap; };
struct Screen { struct ViewPort ViewPort; };
struct RastPort {
 struct Window* window;
 ULONG apen, bpen, drawmode;
 LONG cp_x, cp_y;
};
struct Window {
 WORD LeftEdge, TopEdge, Width, Height;
 BYTE BorderLeft, BorderTop, BorderRight, BorderBottom;
 struct RastPort* RPort;
 struct MsgPort* UserPort;
 struct Screen* WScreen;
 ULONG IDCMPFlags;
 /* Shim-private: the window's RGB frame and stacking. */
 uint8_t* alpha_rgb;
 struct Window* alpha_below;
 char alpha_title[128];
};
struct NewWindow;
struct IntuiMessage {
 struct Message ExecMessage;
 ULONG Class;
 UWORD Code, Qualifier;
 APTR IAddress;
 WORD MouseX, MouseY;
 ULONG Seconds, Micros;
 struct Window* IDCMPWindow;
};
struct IntuitionBase { struct Library LibNode; };
struct GfxBase { struct Library LibNode; };
struct FileRequester { APTR fr_Reserved0; STRPTR fr_File; STRPTR fr_Drawer; };
struct DateStamp { LONG ds_Days, ds_Minute, ds_Tick; };

/* IDCMP classes and codes */
#define IDCMP_MOUSEBUTTONS 0x00000008UL
#define IDCMP_REFRESHWINDOW 0x00000004UL
#define IDCMP_CLOSEWINDOW 0x00000200UL
#define IDCMP_RAWKEY 0x00000400UL
#define IDCMP_VANILLAKEY 0x00200000UL
#define SELECTDOWN 0x68
#define SELECTUP 0xE8
#define MENUDOWN 0x69
#define MENUUP 0xE9
/* Window flags and tags */
#define WFLG_DRAGBAR 0x0002UL
#define WFLG_DEPTHGADGET 0x0004UL
#define WFLG_CLOSEGADGET 0x0008UL
#define WFLG_SMART_REFRESH 0x0000UL
#define WFLG_ACTIVATE 0x1000UL
#define WA_Dummy 0x80000063UL
#define WA_Left (WA_Dummy+0x01)
#define WA_Top (WA_Dummy+0x02)
#define WA_Width (WA_Dummy+0x03)
#define WA_Height (WA_Dummy+0x04)
#define WA_IDCMP (WA_Dummy+0x07)
#define WA_Flags (WA_Dummy+0x08)
#define WA_Title (WA_Dummy+0x0B)
#define WA_InnerWidth (WA_Dummy+0x13)
#define WA_InnerHeight (WA_Dummy+0x14)
/* graphics draw modes */
#define JAM1 0
#define JAM2 1
/* cybergraphics */
#define RECTFMT_RGB 0
/* asl */
#define ASL_FileRequest 0
#define ASL_TB 0x80080000UL
#define ASLFR_Window (ASL_TB+2)
#define ASLFR_TitleText (ASL_TB+1)
#define ASLFR_InitialDrawer (ASL_TB+9)

#ifdef __cplusplus
extern "C" {
#endif
/* exec */
struct Library* OpenLibrary(STRPTR name, ULONG version);
void CloseLibrary(struct Library* library);
struct Message* WaitPort(struct MsgPort* port);
struct Message* GetMsg(struct MsgPort* port);
void ReplyMsg(struct Message* message);
/* intuition */
struct Window* OpenWindowTags(struct NewWindow* unused, ULONG tag, ...);
void CloseWindow(struct Window* window);
void ActivateWindow(struct Window* window);
void BeginRefresh(struct Window* window);
void EndRefresh(struct Window* window, BOOL complete);
/* graphics */
void SetAPen(struct RastPort* rp, ULONG pen);
void SetBPen(struct RastPort* rp, ULONG pen);
void SetDrMd(struct RastPort* rp, ULONG mode);
void Move(struct RastPort* rp, LONG x, LONG y);
void Draw(struct RastPort* rp, LONG x, LONG y);
LONG ObtainBestPen(struct ColorMap* cm, ULONG r, ULONG g, ULONG b, ULONG tag, ...);
void ReleasePen(struct ColorMap* cm, ULONG pen);
/* cybergraphics */
ULONG WritePixelArray(APTR src, UWORD srcx, UWORD srcy, UWORD srcmod, struct RastPort* rp, UWORD dx, UWORD dy, UWORD width, UWORD height, UBYTE format);
ULONG ReadPixelArray(APTR dst, UWORD dstx, UWORD dsty, UWORD dstmod, struct RastPort* rp, UWORD sx, UWORD sy, UWORD width, UWORD height, UBYTE format);
ULONG FillPixelArray(struct RastPort* rp, UWORD x, UWORD y, UWORD width, UWORD height, ULONG argb);
/* asl */
APTR AllocAslRequestTags(ULONG type, ULONG tag, ...);
BOOL AslRequestTags(APTR request, ULONG tag, ...);
void FreeAslRequest(APTR request);
/* dos (only the local-player candidate uses these) */
struct DateStamp* DateStamp(struct DateStamp* stamp);
BPTR CreateDir(STRPTR name);
void UnLock(BPTR lock);
#ifdef __cplusplus
}
#endif
#endif
