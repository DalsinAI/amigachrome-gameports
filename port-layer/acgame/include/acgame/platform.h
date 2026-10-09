#ifndef ACGAME_PLATFORM_H
#define ACGAME_PLATFORM_H

#include "acgame.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACGAME_VIDEO_INDEXED8 = 1,
    ACGAME_VIDEO_HAM8 = 2
} acgame_video_mode;

/* Which kind of 8-bit screen to open. AUTO lets the system pick the best
 * 8-bit mode (an RTG mode where one exists); ENV:ACGame/Display overrides. */
typedef enum {
    ACGAME_DISPLAY_AUTO = 0,
    ACGAME_DISPLAY_AGA = 1,
    ACGAME_DISPLAY_RTG = 2
} acgame_display_kind;

typedef enum {
    ACGAME_KEY_UP = 0,
    ACGAME_KEY_DOWN,
    ACGAME_KEY_LEFT,
    ACGAME_KEY_RIGHT,
    ACGAME_KEY_FIRE1,
    ACGAME_KEY_FIRE2,
    ACGAME_KEY_FIRE3,
    ACGAME_KEY_START,
    ACGAME_KEY_ESCAPE,
    ACGAME_KEY_COUNT
} acgame_key;

/* A game pad in the standard layout (SDL 2's GameController layout, which is
 * OpenInput's): the button and axis numbers are OpenInput's OIB_ and OIAXIS_
 * numbers, so a game needs no OpenInput header to read them. */
enum {
    ACGAME_PAD_A = 0,               /* the bottom face button; a CD32 pad's red */
    ACGAME_PAD_B,                   /* the right face button; CD32 blue */
    ACGAME_PAD_X,                   /* the left face button; CD32 green */
    ACGAME_PAD_Y,                   /* the top face button; CD32 yellow */
    ACGAME_PAD_BACK,
    ACGAME_PAD_GUIDE,
    ACGAME_PAD_START,               /* CD32 play */
    ACGAME_PAD_LEFTSTICK,
    ACGAME_PAD_RIGHTSTICK,
    ACGAME_PAD_LEFTSHOULDER,        /* CD32 reverse */
    ACGAME_PAD_RIGHTSHOULDER,       /* CD32 forward */
    ACGAME_PAD_DPAD_UP,
    ACGAME_PAD_DPAD_DOWN,
    ACGAME_PAD_DPAD_LEFT,
    ACGAME_PAD_DPAD_RIGHT,
    ACGAME_PAD_BUTTON_COUNT = 21
};
#define ACGAME_PAD_BIT(b) (1UL << (b))

enum {
    ACGAME_PAD_LEFTX = 0,           /* -32768 (left) to 32767 (right) */
    ACGAME_PAD_LEFTY,               /* -32768 (up) to 32767 (down) */
    ACGAME_PAD_RIGHTX,
    ACGAME_PAD_RIGHTY,
    ACGAME_PAD_TRIGGERLEFT,         /* 0 (released) to 32767 (fully pulled) */
    ACGAME_PAD_TRIGGERRIGHT,
    ACGAME_PAD_AXIS_COUNT
};

/* Where the pad's state came from. */
typedef enum {
    ACGAME_PAD_NONE = 0,            /* no pad */
    ACGAME_PAD_OPENINPUT = 1,       /* openinput.library 1.2 or later */
    ACGAME_PAD_LOWLEVEL = 2         /* lowlevel.library's ReadJoyPort, port 2 */
} acgame_pad_source;

typedef struct {
    uint8_t connected;
    uint8_t source;                 /* acgame_pad_source */
    uint16_t reserved;
    uint32_t buttons;               /* ACGAME_PAD_BIT(ACGAME_PAD_*) */
    int16_t axes[ACGAME_PAD_AXIS_COUNT];
    const char *name;               /* the pad's name; valid until the next poll */
} acgame_pad_state;

typedef struct {
    uint8_t key[ACGAME_KEY_COUNT];  /* keyboard, with the pad's d-pad, left stick, A, B, X and Start ORed in */
    int16_t mouse_dx;
    int16_t mouse_dy;
    uint8_t mouse_buttons;
    uint8_t quit_requested;
    acgame_pad_state pad;           /* the first pad, in full */
} acgame_input_state;

/* What the backend found when it opened the screen. */
typedef struct {
    int planar;                     /* 1: native AGA planes (ACGame's C2P); 0: chunky (RTG) */
    int opengfx;                    /* 1: OpenGfx (opengpu.library 0.8+) has the drawing calls, so an
                                     * RTG frame is drawn by the cores and the GPU on AmigaChrome */
    int openinput;                  /* openinput.library's (version << 16 | revision), 0 when not used */
    unsigned screen_width;          /* the screen opened; the frame is centred on it */
    unsigned screen_height;
} acgame_platform_info;

typedef struct {
    unsigned width;
    unsigned height;
    unsigned refresh_hz;
    acgame_video_mode mode;
    acgame_display_kind display;
} acgame_video_config;

int acgame_platform_init(const acgame_video_config *config);
void acgame_platform_shutdown(void);
acgame_indexed_surface *acgame_platform_begin_indexed_frame(void);
int acgame_platform_present_indexed(const acgame_indexed_surface *frame);
/* 1 when the open screen is native planar AGA, 0 for RTG, -1 when closed. */
int acgame_platform_display_is_planar(void);
/* 0 and info filled while a screen is open, -1 otherwise. */
int acgame_platform_get_info(acgame_platform_info *info);
int acgame_platform_present_ham8(const uint8_t *ham_codes, size_t pitch,
                                 const acgame_rgb8 base_palette[64]);
void acgame_platform_poll(acgame_input_state *state);
uint32_t acgame_platform_ticks_ms(void);
int acgame_platform_audio_s16stereo(const int16_t *samples, unsigned frames,
                                    unsigned sample_rate);

#ifdef __cplusplus
}
#endif

#endif
