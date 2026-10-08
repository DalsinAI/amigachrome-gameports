#include "acgame/platform.h"

#include <string.h>

#ifdef __AROS__

#include <exec/types.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

typedef struct {
    struct Screen *screen;
    struct Window *window;
    acgame_indexed_surface frame;
    acgame_rgb8 last_palette[256];
    uint8_t *pixels;
    uint8_t keys[ACGAME_KEY_COUNT];
    uint8_t mouse_buttons;
    unsigned width;
    unsigned height;
    int palette_valid;
    int quit_requested;
} acgame_aros_state;

static acgame_aros_state g_acgame_aros;

static void close_display(void) {
    if (g_acgame_aros.window) {
        CloseWindow(g_acgame_aros.window);
        g_acgame_aros.window = NULL;
    }
    if (g_acgame_aros.screen) {
        CloseScreen(g_acgame_aros.screen);
        g_acgame_aros.screen = NULL;
    }
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
    if (!g_acgame_aros.screen) {
        return -1;
    }
    if (g_acgame_aros.palette_valid &&
        memcmp(g_acgame_aros.last_palette, palette, sizeof(g_acgame_aros.last_palette)) == 0) {
        return 0;
    }
    build_palette_table(palette, table);
    LoadRGB32(&g_acgame_aros.screen->ViewPort, table);
    memcpy(g_acgame_aros.last_palette, palette, sizeof(g_acgame_aros.last_palette));
    g_acgame_aros.palette_valid = 1;
    return 0;
}

static int copy_to_planar(const acgame_indexed_surface *frame) {
    struct BitMap *bitmap;
    uint8_t *planes[8];
    unsigned p;
    size_t plane_pitch;

    if (!g_acgame_aros.screen || !frame || !frame->pixels) {
        return -1;
    }
    bitmap = g_acgame_aros.screen->RastPort.BitMap;
    if (!bitmap || bitmap->Depth < 8 || frame->width != g_acgame_aros.width ||
        frame->height != g_acgame_aros.height) {
        return -1;
    }
    plane_pitch = bitmap->BytesPerRow;
    for (p = 0; p < 8; ++p) {
        if (!bitmap->Planes[p]) {
            return -1;
        }
        planes[p] = (uint8_t *)bitmap->Planes[p];
    }
    return acgame_c2p8_ref(frame->pixels, frame->pitch, planes, plane_pitch,
                           frame->width, frame->height);
}

static void set_key(unsigned raw, int down) {
    switch (raw) {
    case 0x4c:
        g_acgame_aros.keys[ACGAME_KEY_UP] = (uint8_t)down;
        break;
    case 0x4d:
        g_acgame_aros.keys[ACGAME_KEY_DOWN] = (uint8_t)down;
        break;
    case 0x4f:
        g_acgame_aros.keys[ACGAME_KEY_LEFT] = (uint8_t)down;
        break;
    case 0x4e:
        g_acgame_aros.keys[ACGAME_KEY_RIGHT] = (uint8_t)down;
        break;
    case 0x40:
        g_acgame_aros.keys[ACGAME_KEY_FIRE1] = (uint8_t)down;
        break;
    case 0x64:
        g_acgame_aros.keys[ACGAME_KEY_FIRE2] = (uint8_t)down;
        break;
    case 0x44:
        g_acgame_aros.keys[ACGAME_KEY_START] = (uint8_t)down;
        break;
    case 0x45:
        g_acgame_aros.keys[ACGAME_KEY_ESCAPE] = (uint8_t)down;
        if (down) {
            g_acgame_aros.quit_requested = 1;
        }
        break;
    default:
        break;
    }
}

int acgame_platform_init(const acgame_video_config *config) {
    ULONG idcmp = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS;
    memset(&g_acgame_aros, 0, sizeof(g_acgame_aros));

    if (!config || config->mode != ACGAME_VIDEO_INDEXED8 ||
        config->width == 0 || config->height == 0 || (config->width & 7u)) {
        return -1;
    }

    g_acgame_aros.screen = OpenScreenTags(
        NULL,
        SA_Type, CUSTOMSCREEN,
        SA_Width, config->width,
        SA_Height, config->height,
        SA_Depth, 8,
        SA_ShowTitle, FALSE,
        SA_Draggable, FALSE,
        SA_Quiet, TRUE,
        TAG_END);
    if (!g_acgame_aros.screen) {
        return -1;
    }

    g_acgame_aros.window = OpenWindowTags(
        NULL,
        WA_CustomScreen, g_acgame_aros.screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, config->width,
        WA_Height, config->height,
        WA_Activate, TRUE,
        WA_Borderless, TRUE,
        WA_Backdrop, TRUE,
        WA_RMBTrap, TRUE,
        WA_NoCareRefresh, TRUE,
        WA_IDCMP, idcmp,
        TAG_END);
    if (!g_acgame_aros.window) {
        close_display();
        return -1;
    }

    g_acgame_aros.pixels = (uint8_t *)malloc((size_t)config->width * config->height);
    if (!g_acgame_aros.pixels) {
        close_display();
        return -1;
    }
    memset(g_acgame_aros.pixels, 0, (size_t)config->width * config->height);

    g_acgame_aros.width = config->width;
    g_acgame_aros.height = config->height;
    g_acgame_aros.frame.width = (uint16_t)config->width;
    g_acgame_aros.frame.height = (uint16_t)config->height;
    g_acgame_aros.frame.pitch = config->width;
    g_acgame_aros.frame.pixels = g_acgame_aros.pixels;
    return 0;
}

void acgame_platform_shutdown(void) {
    free(g_acgame_aros.pixels);
    g_acgame_aros.pixels = NULL;
    close_display();
    memset(&g_acgame_aros, 0, sizeof(g_acgame_aros));
}

acgame_indexed_surface *acgame_platform_begin_indexed_frame(void) {
    return g_acgame_aros.pixels ? &g_acgame_aros.frame : NULL;
}

int acgame_platform_present_indexed(const acgame_indexed_surface *frame) {
    int rc;
    if (!frame || update_palette(frame->palette) != 0) {
        return -1;
    }
    WaitTOF();
    rc = copy_to_planar(frame);
    return rc;
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

    if (g_acgame_aros.window && g_acgame_aros.window->UserPort) {
        while ((message = (struct IntuiMessage *)GetMsg(g_acgame_aros.window->UserPort)) != NULL) {
            if (message->Class == IDCMP_RAWKEY) {
                const unsigned code = (unsigned)message->Code;
                set_key(code & 0x7fu, (code & 0x80u) == 0);
            } else if (message->Class == IDCMP_MOUSEBUTTONS) {
                if (message->Code == SELECTDOWN) {
                    g_acgame_aros.mouse_buttons |= 1;
                } else if (message->Code == SELECTUP) {
                    g_acgame_aros.mouse_buttons &= (uint8_t)~1u;
                } else if (message->Code == MENUDOWN) {
                    g_acgame_aros.mouse_buttons |= 2;
                } else if (message->Code == MENUUP) {
                    g_acgame_aros.mouse_buttons &= (uint8_t)~2u;
                }
            }
            ReplyMsg((struct Message *)message);
        }
    }

    memcpy(state->key, g_acgame_aros.keys, sizeof(state->key));
    state->mouse_buttons = g_acgame_aros.mouse_buttons;
    state->quit_requested = (uint8_t)g_acgame_aros.quit_requested;
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

