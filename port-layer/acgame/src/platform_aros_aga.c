#include "acgame/platform.h"

#include <string.h>

/* One backend for AmigaOS 3.x (GCC 16.2, libnix) and AROS m68k. The screen
 * is opened through Intuition; a planar 8-plane screen (native AGA) takes the
 * frame through ACGame's C2P, any other 8-bit screen (OpenRTG, or another RTG
 * system) through graphics.library's WriteChunkyPixels, which OpenGfx
 * (opengpu.library 0.8 and later) takes to the cores on AmigaChrome.
 *
 * Pads: openinput.library 1.2 or later when it is there (hot-plug, analogue
 * sticks and triggers, the pad opened exclusively while the game runs), and
 * lowlevel.library's ReadJoyPort on port 2 when it is missing or is the 1.1
 * skeleton. No library is needed: without both, keyboard and mouse remain. */
#if defined(__AROS__) || defined(__amigaos__) || defined(AMIGA)

/* The library bases have ACGame's own names, so a game that links ACGame and
 * SDL 2 (which has a LowLevelBase of its own) or opens a library itself has
 * no clash over them. */
#define LowLevelBase acgame_LowLevelBase
#define OpenInputBase acgame_OpenInputBase
#define OpenGPUBase acgame_OpenGPUBase

#include <exec/types.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <libraries/lowlevel.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/lowlevel.h>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* OpenInput's frozen header (openamigainput include/, copied unchanged into
 * include/acgame/openinput so ACGame builds with nothing else installed). */
#ifndef ACGAME_OPENINPUT
#define ACGAME_OPENINPUT 1
#endif
#if ACGAME_OPENINPUT
#include "acgame/openinput/libraries/openinput.h"
#ifdef __AROS__
/* AROS m68k calls the same library with its own macros (LVO / 6). */
#include <aros/libcall.h>
#define OIN_ListControllers(buffer, max, infoSize) \
    AROS_LC3(ULONG, OIN_ListControllers, AROS_LCA(struct OIControllerInfo *, (buffer), A0), \
             AROS_LCA(ULONG, (max), D0), AROS_LCA(ULONG, (infoSize), D1), \
             struct Library *, OpenInputBase, 5, OpenInput)
#define OIN_OpenControllerA(id, tags) \
    AROS_LC2(APTR, OIN_OpenControllerA, AROS_LCA(ULONG, (id), D0), \
             AROS_LCA(struct TagItem *, (tags), A0), struct Library *, OpenInputBase, 7, OpenInput)
#define OIN_CloseController(controller) \
    AROS_LC1NR(void, OIN_CloseController, AROS_LCA(APTR, (controller), A0), \
               struct Library *, OpenInputBase, 8, OpenInput)
#define OIN_ReadState(controller, state, stateSize) \
    AROS_LC3(LONG, OIN_ReadState, AROS_LCA(APTR, (controller), A0), \
             AROS_LCA(struct OIState *, (state), A1), AROS_LCA(ULONG, (stateSize), D0), \
             struct Library *, OpenInputBase, 9, OpenInput)
#define OIN_AddNotifyA(port, tags) \
    AROS_LC2(APTR, OIN_AddNotifyA, AROS_LCA(struct MsgPort *, (port), A0), \
             AROS_LCA(struct TagItem *, (tags), A1), struct Library *, OpenInputBase, 12, OpenInput)
#define OIN_RemNotify(notify) \
    AROS_LC1NR(void, OIN_RemNotify, AROS_LCA(APTR, (notify), A0), \
               struct Library *, OpenInputBase, 13, OpenInput)
#else
#define OPENINPUT_BASE_NAME OpenInputBase
#define NO_INLINE_STDARG
#include "acgame/openinput/inline/openinput.h"
#endif
#endif

/* OpenGPU SDK 0.7 (the os32-gcc16 stove): only to ask whether OpenGfx has
 * the drawing calls. Builds without the SDK (AROS) leave it out. */
#ifndef ACGAME_OPENGPU
#if defined(__has_include) && !defined(__AROS__)
#if __has_include(<proto/opengpu.h>)
#define ACGAME_OPENGPU 1
#endif
#endif
#endif
#ifndef ACGAME_OPENGPU
#define ACGAME_OPENGPU 0
#endif
#if ACGAME_OPENGPU
#define OPENGPU_BASE_NAME OpenGPUBase
#include <opengpu/opengpu.h>
#include <opengpu/gfx.h>
#include <inline/opengpu.h>
#endif

/* Opened here, not by the startup code, so a machine without them still
 * runs with keyboard and mouse only. */
struct Library *LowLevelBase = NULL;
#if ACGAME_OPENINPUT
struct Library *OpenInputBase = NULL;
#endif
#if ACGAME_OPENGPU
struct Library *OpenGPUBase = NULL;
#endif

#if ACGAME_OPENINPUT
#define ACGAME_OI_MAX 8
typedef struct {
    struct MsgPort *port;           /* OpenInput's ADDED and REMOVED messages */
    APTR notify;
    APTR handle;                    /* the pad in use, opened exclusively when it could be */
    ULONG id;
    ULONG flags;                    /* its OICF_ flags */
    int rescan;                     /* look for a (better) pad at the next poll */
    char name[64];
} acgame_openinput;
#endif

typedef struct {
    struct Screen *screen;
    struct Window *window;
    acgame_indexed_surface frame;
    acgame_rgb8 last_palette[256];
    uint8_t *pixels;
    uint8_t keys[ACGAME_KEY_COUNT];
    uint8_t mouse_buttons;
    int16_t mouse_dx;
    int16_t mouse_dy;
    unsigned width;
    unsigned height;
    unsigned left;  /* where the frame sits on a larger board screen */
    unsigned top;
    int planar;
    int opengfx;
    int palette_valid;
    int quit_requested;
    unsigned screen_width;
    unsigned screen_height;
#if ACGAME_OPENINPUT
    acgame_openinput oi;
#endif
} acgame_amiga_state;

static acgame_amiga_state g_acgame_amiga;

/* ---- pads ------------------------------------------------------------------ */

/* ENV:ACGame/<name> as a string; 0 when it is not set. */
static int env_value(const char *name, char *value, LONG size) {
    return GetVar((STRPTR)name, (STRPTR)value, size, 0) > 0;
}

#if ACGAME_OPENINPUT
static void oi_close_pad(acgame_openinput *oi) {
    if (oi->handle) {
        OIN_CloseController(oi->handle);    /* gives back an exclusive pad's legacy feed */
        oi->handle = NULL;
    }
    oi->id = 0;
    oi->flags = 0;
    oi->name[0] = 0;
}

static void oi_close(acgame_openinput *oi) {
    oi_close_pad(oi);
    if (oi->notify) {
        OIN_RemNotify(oi->notify);          /* takes back its messages still in the port */
        oi->notify = NULL;
    }
    if (oi->port) {
        DeleteMsgPort(oi->port);
        oi->port = NULL;
    }
    if (OpenInputBase) {
        CloseLibrary(OpenInputBase);
        OpenInputBase = NULL;
    }
}

/* openinput.library 1.2 or later; the 1.1 skeleton lists nothing, so it
 * counts as missing. ENV:ACGame/OpenInput 0 leaves it alone (lowlevel only). */
static void oi_open(acgame_openinput *oi) {
    char value[8];
    ULONG tags[3];
    struct Library *lib;
    memset(oi, 0, sizeof(*oi));
    if (env_value("ACGame/OpenInput", value, sizeof(value)) && value[0] == '0') {
        return;
    }
    lib = OpenLibrary((CONST_STRPTR)OPENINPUT_NAME, OPENINPUT_VERSION);
    if (!lib) {
        return;
    }
    if (lib->lib_Version == OPENINPUT_VERSION && lib->lib_Revision < OPENINPUT_REVISION_CORE) {
        CloseLibrary(lib);
        return;
    }
    OpenInputBase = lib;
    oi->port = CreateMsgPort();
    if (oi->port) {
        tags[0] = OIT_Events;
        tags[1] = OIMC_ADDED | OIMC_REMOVED;
        tags[2] = TAG_END;
        oi->notify = OIN_AddNotifyA(oi->port, (struct TagItem *)tags);
    }
    oi->rescan = 1;
}

/* Which pad to take: a plugged-in pad (one that can go away: USB, or a pad on
 * AmigaChrome's cores) before the Amiga's own joystick port, and port 2 (the
 * joystick port) before the others. One another program has exclusively is
 * passed over. */
static int oi_rank(const struct OIControllerInfo *info) {
    if (info->oci_Flags & OICF_INUSE) {
        return 0;
    }
    if (info->oci_Flags & OICF_REMOVABLE) {
        return 3;
    }
    return info->oci_LegacyPort == 1 ? 2 : 1;
}

static void oi_pick(acgame_openinput *oi) {
    struct OIControllerInfo list[ACGAME_OI_MAX];
    ULONG n, i, tags[5];
    int tries;
    oi->rescan = 0;
    n = OIN_ListControllers(list, ACGAME_OI_MAX, sizeof(list[0]));
    if (n > ACGAME_OI_MAX) {
        n = ACGAME_OI_MAX;
    }
    if (oi->handle) {
        /* Keep the pad in use unless a plugged-in one has come and this is
         * the Amiga's own port. */
        int better = 0;
        if (oi->flags & OICF_REMOVABLE) {
            return;
        }
        for (i = 0; i < n; ++i) {
            if (list[i].oci_ID != oi->id && oi_rank(&list[i]) == 3) {
                better = 1;
            }
        }
        if (!better) {
            return;
        }
        oi_close_pad(oi);
    }
    for (tries = 0; tries < ACGAME_OI_MAX; ++tries) {
        ULONG best = n;
        int best_rank = 0;
        for (i = 0; i < n; ++i) {
            const int rank = oi_rank(&list[i]);
            if (rank > best_rank) {
                best_rank = rank;
                best = i;
            }
        }
        if (best == n) {
            return;
        }
        /* Exclusive while the game runs: the pad stops driving its Amiga port
         * for older programs until it is closed. */
        tags[0] = OIT_Exclusive;
        tags[1] = TRUE;
        tags[2] = OIT_Player;
        tags[3] = 0;
        tags[4] = TAG_END;
        oi->handle = OIN_OpenControllerA(list[best].oci_ID, (struct TagItem *)tags);
        if (oi->handle) {
            oi->id = list[best].oci_ID;
            oi->flags = list[best].oci_Flags;
            memcpy(oi->name, list[best].oci_Name, sizeof(oi->name));
            oi->name[sizeof(oi->name) - 1] = 0;
            return;
        }
        list[best].oci_Flags |= OICF_INUSE;     /* try the next one */
    }
}

/* 1 when the pad came through OpenInput; 0 leaves it to lowlevel.library. */
static int oi_read(acgame_openinput *oi, acgame_pad_state *pad) {
    struct OIState st;
    struct OIMessage *msg;
    if (!OpenInputBase) {
        return 0;
    }
    if (oi->port) {
        while ((msg = (struct OIMessage *)GetMsg(oi->port)) != NULL) {
            if (msg->oim_Class & (OIMC_ADDED | OIMC_REMOVED)) {
                oi->rescan = 1;
            }
            ReplyMsg((struct Message *)msg);
        }
    } else if (!oi->handle) {
        oi->rescan = 1;     /* no notification: look again at each poll */
    }
    if (oi->rescan) {
        oi_pick(oi);
    }
    if (!oi->handle) {
        return 0;
    }
    if (OIN_ReadState(oi->handle, &st, sizeof(st)) != OIERR_OK || !(st.ois_Flags & OISF_CONNECTED)) {
        oi_close_pad(oi);   /* it went: the next poll takes another */
        oi->rescan = 1;
        return 0;
    }
    pad->connected = 1;
    pad->source = ACGAME_PAD_OPENINPUT;
    pad->buttons = (uint32_t)st.ois_Buttons;
    memcpy(pad->axes, st.ois_Axes, sizeof(pad->axes));
    pad->name = oi->name;
    return 1;
}
#endif

/* Joystick or CD32 pad in port 2 (lowlevel unit 1), in the standard layout. */
static void lowlevel_read(acgame_pad_state *pad) {
    ULONG joy, type;
    uint32_t b = 0;
    if (!LowLevelBase) {
        return;
    }
    joy = ReadJoyPort(1);
    type = joy & JP_TYPE_MASK;
    if (type == JP_TYPE_NOTAVAIL || type == JP_TYPE_MOUSE) {
        return;
    }
    if (joy & JPF_JOY_UP) b |= ACGAME_PAD_BIT(ACGAME_PAD_DPAD_UP);
    if (joy & JPF_JOY_DOWN) b |= ACGAME_PAD_BIT(ACGAME_PAD_DPAD_DOWN);
    if (joy & JPF_JOY_LEFT) b |= ACGAME_PAD_BIT(ACGAME_PAD_DPAD_LEFT);
    if (joy & JPF_JOY_RIGHT) b |= ACGAME_PAD_BIT(ACGAME_PAD_DPAD_RIGHT);
    if (joy & JPF_BUTTON_RED) b |= ACGAME_PAD_BIT(ACGAME_PAD_A);
    if (joy & JPF_BUTTON_BLUE) b |= ACGAME_PAD_BIT(ACGAME_PAD_B);
    if (type == JP_TYPE_GAMECTLR) {
        if (joy & JPF_BUTTON_GREEN) b |= ACGAME_PAD_BIT(ACGAME_PAD_X);
        if (joy & JPF_BUTTON_YELLOW) b |= ACGAME_PAD_BIT(ACGAME_PAD_Y);
        if (joy & JPF_BUTTON_PLAY) b |= ACGAME_PAD_BIT(ACGAME_PAD_START);
        if (joy & JPF_BUTTON_REVERSE) b |= ACGAME_PAD_BIT(ACGAME_PAD_LEFTSHOULDER);
        if (joy & JPF_BUTTON_FORWARD) b |= ACGAME_PAD_BIT(ACGAME_PAD_RIGHTSHOULDER);
    }
    if (type == JP_TYPE_UNKNOWN && !b) {
        return;     /* nothing seen in the port yet */
    }
    pad->connected = 1;
    pad->source = ACGAME_PAD_LOWLEVEL;
    pad->buttons = b;
    pad->axes[ACGAME_PAD_LEFTX] = (int16_t)((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_LEFT)) ? -32768 :
                                            (b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_RIGHT)) ? 32767 : 0);
    pad->axes[ACGAME_PAD_LEFTY] = (int16_t)((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_UP)) ? -32768 :
                                            (b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_DOWN)) ? 32767 : 0);
    pad->name = type == JP_TYPE_GAMECTLR ? "CD32 pad, port 2" : "Joystick, port 2";
}

/* The pad's d-pad or left stick, A, B, X and Start, ORed over the keyboard. */
static void pad_to_keys(const acgame_pad_state *pad, acgame_input_state *state) {
    const uint32_t b = pad->buttons;
    const int16_t lx = pad->axes[ACGAME_PAD_LEFTX];
    const int16_t ly = pad->axes[ACGAME_PAD_LEFTY];
    if (!pad->connected) {
        return;
    }
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_UP)) || ly < -16384) state->key[ACGAME_KEY_UP] = 1;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_DOWN)) || ly > 16384) state->key[ACGAME_KEY_DOWN] = 1;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_LEFT)) || lx < -16384) state->key[ACGAME_KEY_LEFT] = 1;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_RIGHT)) || lx > 16384) state->key[ACGAME_KEY_RIGHT] = 1;
    if (b & ACGAME_PAD_BIT(ACGAME_PAD_A)) state->key[ACGAME_KEY_FIRE1] = 1;
    if (b & ACGAME_PAD_BIT(ACGAME_PAD_B)) state->key[ACGAME_KEY_FIRE2] = 1;
    if (b & ACGAME_PAD_BIT(ACGAME_PAD_X)) state->key[ACGAME_KEY_FIRE3] = 1;
    if (b & ACGAME_PAD_BIT(ACGAME_PAD_START)) state->key[ACGAME_KEY_START] = 1;
}

static void close_input(void) {
#if ACGAME_OPENINPUT
    oi_close(&g_acgame_amiga.oi);
#endif
    if (LowLevelBase) {
        CloseLibrary(LowLevelBase);
        LowLevelBase = NULL;
    }
}

/* ---- display ---------------------------------------------------------------- */

static void close_display(void) {
    if (g_acgame_amiga.window) {
        CloseWindow(g_acgame_amiga.window);
        g_acgame_amiga.window = NULL;
    }
    if (g_acgame_amiga.screen) {
        CloseScreen(g_acgame_amiga.screen);
        g_acgame_amiga.screen = NULL;
    }
#if ACGAME_OPENGPU
    if (OpenGPUBase) {
        CloseLibrary(OpenGPUBase);
        OpenGPUBase = NULL;
    }
#endif
}

/* Whether OpenGfx has graphics.library's drawing calls (opengpu.library 0.8
 * and later, patches in): then WriteChunkyPixels on an RTG screen is done by
 * the cores, not the 68k. A planar screen still takes ACGame's own C2P, as
 * OpenGfx has no planar path for WriteChunkyPixels yet. */
static int opengfx_active(void) {
#if ACGAME_OPENGPU
    if (!OpenGPUBase) {
        OpenGPUBase = OpenLibrary((CONST_STRPTR)OPENGPU_NAME, OPENGPU_VERSION);
    }
    if (!OpenGPUBase || OpenGPUBase->lib_Revision < OPENGPU_OGFX_ALL_REVISION) {
        return 0;
    }
    return (OGFX_Status() & (OGFX_STATUS_PATCHED | OGFX_STATUS_ENABLED)) ==
           (OGFX_STATUS_PATCHED | OGFX_STATUS_ENABLED);
#else
    return 0;
#endif
}

/* ENV:ACGame/Display (AGA, RTG or AUTO) overrides what the game asked for, so
 * a player can push a game onto the other kind of screen without a rebuild. */
static acgame_display_kind display_from_env(acgame_display_kind wanted) {
    char value[16];
    if (!env_value("ACGame/Display", value, sizeof(value))) {
        return wanted;
    }
    if (value[0] == 'A' || value[0] == 'a') {
        return (value[1] == 'G' || value[1] == 'g') ? ACGAME_DISPLAY_AGA : ACGAME_DISPLAY_AUTO;
    }
    if (value[0] == 'R' || value[0] == 'r') {
        return ACGAME_DISPLAY_RTG;
    }
    return wanted;
}

static ULONG pick_mode(const acgame_video_config *config, acgame_display_kind kind) {
    if (kind == ACGAME_DISPLAY_AGA) {
        /* The default custom screen on the native chipset: lores, PAL or NTSC
         * as the machine boots. */
        return INVALID_ID;
    }
    return BestModeID(BIDTAG_NominalWidth, config->width,
                      BIDTAG_NominalHeight, config->height,
                      BIDTAG_DesiredWidth, config->width,
                      BIDTAG_DesiredHeight, config->height,
                      BIDTAG_Depth, 8,
                      TAG_END);
}

/* The PropertyFlags bit Picasso96 and OpenRTG set on a board's modes. */
#define ACGAME_DIPF_BOARD 0x02000000UL

static int is_board_mode(ULONG mode) {
    struct DisplayInfo info;
    if (mode == (ULONG)INVALID_ID ||
        !GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_DISP, mode)) {
        return 0;
    }
    return (info.PropertyFlags & (ACGAME_DIPF_BOARD | DIPF_IS_FOREIGN)) != 0;
}

/* BestModeID passes over OpenRTG's modes (OpenRTG 0.7, 6 Oct 2026), and a
 * board may have no mode of the frame's own size, so walk the database for
 * the smallest 8-bit board mode that holds the frame. */
static ULONG find_board_mode(const acgame_video_config *config, unsigned *width, unsigned *height) {
    ULONG mode = INVALID_ID;
    ULONG best = INVALID_ID;
    unsigned long best_area = 0;
    while ((mode = NextDisplayInfo(mode)) != (ULONG)INVALID_ID) {
        struct DimensionInfo dims;
        unsigned w, h;
        if (!is_board_mode(mode) ||
            !GetDisplayInfoData(NULL, (UBYTE *)&dims, sizeof(dims), DTAG_DIMS, mode) ||
            dims.MaxDepth != 8) {
            continue;
        }
        w = (unsigned)(dims.Nominal.MaxX - dims.Nominal.MinX + 1);
        h = (unsigned)(dims.Nominal.MaxY - dims.Nominal.MinY + 1);
        if (w < config->width || h < config->height) {
            continue;
        }
        if (best == (ULONG)INVALID_ID || (unsigned long)w * h < best_area) {
            best = mode;
            best_area = (unsigned long)w * h;
            *width = w;
            *height = h;
        }
    }
    return best;
}

static struct Screen *open_screen(const acgame_video_config *config, ULONG mode) {
    if (mode == (ULONG)INVALID_ID) {
        return OpenScreenTags(NULL,
                              SA_Type, CUSTOMSCREEN,
                              SA_Width, config->width,
                              SA_Height, config->height,
                              SA_Depth, 8,
                              SA_ShowTitle, FALSE,
                              SA_Draggable, FALSE,
                              SA_Quiet, TRUE,
                              TAG_END);
    }
    return OpenScreenTags(NULL,
                          SA_Type, CUSTOMSCREEN,
                          SA_DisplayID, mode,
                          SA_Width, config->width,
                          SA_Height, config->height,
                          SA_Depth, 8,
                          SA_ShowTitle, FALSE,
                          SA_Draggable, FALSE,
                          SA_Quiet, TRUE,
                          TAG_END);
}

static int bitmap_is_planar8(struct BitMap *bitmap) {
    unsigned p;
    if (!bitmap || GetBitMapAttr(bitmap, BMA_DEPTH) < 8 ||
        !(GetBitMapAttr(bitmap, BMA_FLAGS) & BMF_STANDARD)) {
        return 0;
    }
    for (p = 0; p < 8; ++p) {
        if (!bitmap->Planes[p]) {
            return 0;
        }
    }
    return 1;
}

static void build_palette_table(const acgame_rgb8 palette[256], ULONG table[770]) {
    unsigned i;
    table[0] = (256UL << 16);
    for (i = 0; i < 256; ++i) {
        table[1 + i * 3 + 0] = (ULONG)palette[i].r * 0x01010101UL;
        table[1 + i * 3 + 1] = (ULONG)palette[i].g * 0x01010101UL;
        table[1 + i * 3 + 2] = (ULONG)palette[i].b * 0x01010101UL;
    }
    table[769] = 0;
}

static int update_palette(const acgame_rgb8 palette[256]) {
    ULONG table[770];
    if (!g_acgame_amiga.screen) {
        return -1;
    }
    if (g_acgame_amiga.palette_valid &&
        memcmp(g_acgame_amiga.last_palette, palette, sizeof(g_acgame_amiga.last_palette)) == 0) {
        return 0;
    }
    build_palette_table(palette, table);
    LoadRGB32(&g_acgame_amiga.screen->ViewPort, table);
    memcpy(g_acgame_amiga.last_palette, palette, sizeof(g_acgame_amiga.last_palette));
    g_acgame_amiga.palette_valid = 1;
    return 0;
}

static int copy_to_screen(const acgame_indexed_surface *frame) {
    struct BitMap *bitmap;
    uint8_t *planes[8];
    unsigned p;

    if (!g_acgame_amiga.screen || !frame || !frame->pixels ||
        frame->width != g_acgame_amiga.width || frame->height != g_acgame_amiga.height) {
        return -1;
    }
    if (!g_acgame_amiga.planar) {
        const unsigned x = g_acgame_amiga.left;
        const unsigned y = g_acgame_amiga.top;
        WriteChunkyPixels(&g_acgame_amiga.screen->RastPort, x, y,
                          x + frame->width - 1, y + frame->height - 1,
                          frame->pixels, (LONG)frame->pitch);
        return 0;
    }
    bitmap = g_acgame_amiga.screen->RastPort.BitMap;
    for (p = 0; p < 8; ++p) {
        planes[p] = (uint8_t *)bitmap->Planes[p];
    }
    return acgame_c2p8_fast(frame->pixels, frame->pitch, planes, bitmap->BytesPerRow,
                            frame->width, frame->height);
}

static void set_key(unsigned raw, int down) {
    switch (raw) {
    case 0x4c:
        g_acgame_amiga.keys[ACGAME_KEY_UP] = (uint8_t)down;
        break;
    case 0x4d:
        g_acgame_amiga.keys[ACGAME_KEY_DOWN] = (uint8_t)down;
        break;
    case 0x4f:
        g_acgame_amiga.keys[ACGAME_KEY_LEFT] = (uint8_t)down;
        break;
    case 0x4e:
        g_acgame_amiga.keys[ACGAME_KEY_RIGHT] = (uint8_t)down;
        break;
    case 0x40:
        g_acgame_amiga.keys[ACGAME_KEY_FIRE1] = (uint8_t)down;
        break;
    case 0x64:
        g_acgame_amiga.keys[ACGAME_KEY_FIRE2] = (uint8_t)down;
        break;
    case 0x44:
        g_acgame_amiga.keys[ACGAME_KEY_START] = (uint8_t)down;
        break;
    case 0x45:
        g_acgame_amiga.keys[ACGAME_KEY_ESCAPE] = (uint8_t)down;
        if (down) {
            g_acgame_amiga.quit_requested = 1;
        }
        break;
    default:
        break;
    }
}

int acgame_platform_init(const acgame_video_config *config) {
    /* DELTAMOVE: relative mouse movement for games that steer with it. */
    ULONG idcmp = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE;
    acgame_display_kind kind;
    acgame_video_config screen_config;
    ULONG mode;

    memset(&g_acgame_amiga, 0, sizeof(g_acgame_amiga));

    if (!config || config->mode != ACGAME_VIDEO_INDEXED8 ||
        config->width == 0 || config->height == 0 || (config->width & 7u)) {
        return -1;
    }

    kind = display_from_env(config->display);
    mode = pick_mode(config, kind);
    screen_config = *config;
    if (kind == ACGAME_DISPLAY_RTG && !is_board_mode(mode)) {
        unsigned w = 0, h = 0;
        mode = find_board_mode(config, &w, &h);
        if (mode != (ULONG)INVALID_ID) {
            screen_config.width = w;
            screen_config.height = h;
        }
    }
    g_acgame_amiga.screen = open_screen(&screen_config, mode);
    if (!g_acgame_amiga.screen && mode != (ULONG)INVALID_ID && kind == ACGAME_DISPLAY_AUTO) {
        screen_config = *config;
        g_acgame_amiga.screen = open_screen(&screen_config, INVALID_ID);
    }
    if (!g_acgame_amiga.screen) {
        return -1;
    }

    g_acgame_amiga.planar = bitmap_is_planar8(g_acgame_amiga.screen->RastPort.BitMap);
    if ((kind == ACGAME_DISPLAY_AGA && !g_acgame_amiga.planar) ||
        (kind == ACGAME_DISPLAY_RTG && g_acgame_amiga.planar) ||
        (!g_acgame_amiga.planar && ((struct Library *)GfxBase)->lib_Version < 40)) {
        /* WriteChunkyPixels is graphics V40 (OS 3.1). */
        close_display();
        return -1;
    }

    g_acgame_amiga.window = OpenWindowTags(
        NULL,
        WA_CustomScreen, g_acgame_amiga.screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, screen_config.width,
        WA_Height, screen_config.height,
        WA_Activate, TRUE,
        WA_Borderless, TRUE,
        WA_Backdrop, TRUE,
        WA_RMBTrap, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_ReportMouse, TRUE,
        WA_IDCMP, idcmp,
        TAG_END);
    if (!g_acgame_amiga.window) {
        close_display();
        return -1;
    }
    g_acgame_amiga.pixels = (uint8_t *)malloc((size_t)config->width * config->height);
    if (!g_acgame_amiga.pixels) {
        close_display();
        return -1;
    }
    memset(g_acgame_amiga.pixels, 0, (size_t)config->width * config->height);

    g_acgame_amiga.opengfx = opengfx_active();
    LowLevelBase = OpenLibrary((CONST_STRPTR)"lowlevel.library", 40);
#if ACGAME_OPENINPUT
    oi_open(&g_acgame_amiga.oi);
#endif

    g_acgame_amiga.screen_width = screen_config.width;
    g_acgame_amiga.screen_height = screen_config.height;
    g_acgame_amiga.width = config->width;
    g_acgame_amiga.height = config->height;
    g_acgame_amiga.left = (screen_config.width - config->width) / 2u;
    g_acgame_amiga.top = (screen_config.height - config->height) / 2u;
    g_acgame_amiga.frame.width = (uint16_t)config->width;
    g_acgame_amiga.frame.height = (uint16_t)config->height;
    g_acgame_amiga.frame.pitch = config->width;
    g_acgame_amiga.frame.pixels = g_acgame_amiga.pixels;
    return 0;
}

void acgame_platform_shutdown(void) {
    free(g_acgame_amiga.pixels);
    g_acgame_amiga.pixels = NULL;
    close_input();
    close_display();
    memset(&g_acgame_amiga, 0, sizeof(g_acgame_amiga));
}

acgame_indexed_surface *acgame_platform_begin_indexed_frame(void) {
    return g_acgame_amiga.pixels ? &g_acgame_amiga.frame : NULL;
}

int acgame_platform_present_indexed(const acgame_indexed_surface *frame) {
    if (!frame || update_palette(frame->palette) != 0) {
        return -1;
    }
    WaitTOF();
    return copy_to_screen(frame);
}

int acgame_platform_display_is_planar(void) {
    return g_acgame_amiga.screen ? g_acgame_amiga.planar : -1;
}

int acgame_platform_get_info(acgame_platform_info *info) {
    if (!info || !g_acgame_amiga.screen) {
        return -1;
    }
    memset(info, 0, sizeof(*info));
    info->planar = g_acgame_amiga.planar;
    info->opengfx = g_acgame_amiga.opengfx;
#if ACGAME_OPENINPUT
    if (OpenInputBase) {
        info->openinput = ((int)OpenInputBase->lib_Version << 16) | (int)OpenInputBase->lib_Revision;
    }
#endif
    info->screen_width = g_acgame_amiga.screen_width;
    info->screen_height = g_acgame_amiga.screen_height;
    return 0;
}

int acgame_platform_present_ham8(const uint8_t *ham_codes, size_t pitch,
                                 const acgame_rgb8 base_palette[64]) {
    (void)ham_codes;
    (void)pitch;
    (void)base_palette;
    return -1;
}

void acgame_platform_poll(acgame_input_state *state) {
    struct IntuiMessage *message;
    if (!state) {
        return;
    }
    memset(state, 0, sizeof(*state));
    g_acgame_amiga.mouse_dx = 0;
    g_acgame_amiga.mouse_dy = 0;

    if (g_acgame_amiga.window && g_acgame_amiga.window->UserPort) {
        while ((message = (struct IntuiMessage *)GetMsg(g_acgame_amiga.window->UserPort)) != NULL) {
            if (message->Class == IDCMP_RAWKEY) {
                const unsigned code = (unsigned)message->Code;
                set_key(code & 0x7fu, (code & 0x80u) == 0);
            } else if (message->Class == IDCMP_MOUSEMOVE) {
                g_acgame_amiga.mouse_dx = (int16_t)(g_acgame_amiga.mouse_dx + message->MouseX);
                g_acgame_amiga.mouse_dy = (int16_t)(g_acgame_amiga.mouse_dy + message->MouseY);
            } else if (message->Class == IDCMP_MOUSEBUTTONS) {
                if (message->Code == SELECTDOWN) {
                    g_acgame_amiga.mouse_buttons |= 1;
                } else if (message->Code == SELECTUP) {
                    g_acgame_amiga.mouse_buttons &= (uint8_t)~1u;
                } else if (message->Code == MENUDOWN) {
                    g_acgame_amiga.mouse_buttons |= 2;
                } else if (message->Code == MENUUP) {
                    g_acgame_amiga.mouse_buttons &= (uint8_t)~2u;
                }
            }
            ReplyMsg((struct Message *)message);
        }
    }

    memcpy(state->key, g_acgame_amiga.keys, sizeof(state->key));
#if ACGAME_OPENINPUT
    if (!oi_read(&g_acgame_amiga.oi, &state->pad)) {
        lowlevel_read(&state->pad);
    }
#else
    lowlevel_read(&state->pad);
#endif
    pad_to_keys(&state->pad, state);
    state->mouse_dx = g_acgame_amiga.mouse_dx;
    state->mouse_dy = g_acgame_amiga.mouse_dy;
    state->mouse_buttons = g_acgame_amiga.mouse_buttons;
    state->quit_requested = (uint8_t)g_acgame_amiga.quit_requested;
}

uint32_t acgame_platform_ticks_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((uint32_t)tv.tv_sec * 1000u + (uint32_t)(tv.tv_usec / 1000));
}

int acgame_platform_audio_s16stereo(const int16_t *samples, unsigned frames,
                                    unsigned sample_rate) {
    (void)samples;
    (void)frames;
    (void)sample_rate;
    return -1;
}

#else

int acgame_platform_init(const acgame_video_config *config) {
    (void)config;
    return -1;
}
void acgame_platform_shutdown(void) {}
acgame_indexed_surface *acgame_platform_begin_indexed_frame(void) { return NULL; }
int acgame_platform_present_indexed(const acgame_indexed_surface *frame) {
    (void)frame;
    return -1;
}
int acgame_platform_display_is_planar(void) { return -1; }
int acgame_platform_get_info(acgame_platform_info *info) {
    (void)info;
    return -1;
}
int acgame_platform_present_ham8(const uint8_t *ham_codes, size_t pitch,
                                 const acgame_rgb8 base_palette[64]) {
    (void)ham_codes;
    (void)pitch;
    (void)base_palette;
    return -1;
}
void acgame_platform_poll(acgame_input_state *state) {
    if (state) {
        memset(state, 0, sizeof(*state));
    }
}
uint32_t acgame_platform_ticks_ms(void) { return 0; }
int acgame_platform_audio_s16stereo(const int16_t *samples, unsigned frames,
                                    unsigned sample_rate) {
    (void)samples;
    (void)frames;
    (void)sample_rate;
    return -1;
}

#endif
