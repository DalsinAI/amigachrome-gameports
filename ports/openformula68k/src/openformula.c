/* OpenFormula 68K - an original RTG grand-prix racing game for AmigaOS 3.x.
 * SPDX-License-Identifier: MIT
 *
 * The game intentionally uses no licensed teams, cars, circuits or branding.
 * It renders an indexed 8-bit pseudo-3D road through ACGame's native Amiga
 * backend. On an RTG screen that becomes one WriteChunkyPixels operation per
 * frame (OpenGfx/OpenGPU can accelerate it); native AGA remains available as
 * a useful compatibility fallback, though the supported release target is RTG.
 */
#include "acgame/platform.h"
#include "audio_ahi.h"

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char version[] __attribute__((used)) =
    "$VER: OpenFormula68K 0.1.0 (12.10.2026) Dalsin Limited";

unsigned long __stack = 512UL * 1024UL;

#define GAME_W 640
#define GAME_H 360
#define FRAME_HZ 50
#define SEG_LEN 200
#define TRACK_SEGMENTS 1024
#define DRAW_DISTANCE 150
#define ROAD_HALF 900
#define CAMERA_HEIGHT 800
#define MAX_CARS 12
#define PLAYER_INDEX 0
#define MAX_SPEED 12000
#define MAX_LAPS 9
#define DEFAULT_LAPS 3
#define HORIZON 176
#define Q16_ONE 65536

#define COL_BLACK 0
#define COL_SKY0 1
#define COL_SKY1 2
#define COL_SKY2 3
#define COL_CLOUD 4
#define COL_MOUNTAIN0 5
#define COL_MOUNTAIN1 6
#define COL_GRASS0 10
#define COL_GRASS1 11
#define COL_GRASS2 12
#define COL_ROAD0 20
#define COL_ROAD1 21
#define COL_RUMBLE_RED 22
#define COL_RUMBLE_WHITE 23
#define COL_LANE 24
#define COL_SHADOW 25
#define COL_WHITE 30
#define COL_GREY 31
#define COL_DARKGREY 32
#define COL_YELLOW 33
#define COL_ORANGE 34
#define COL_RED 35
#define COL_GREEN 36
#define COL_CYAN 37
#define COL_BLUE 38
#define COL_MAGENTA 39
#define COL_PANEL 40
#define COL_PANEL2 41
#define COL_WHEEL 42
#define COL_GLASS 43
#define COL_SIGN 44
#define COL_SIGN_TEXT 45
#define CAR_COLOR_BASE 64

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* ---- tiny 5x7 font ---------------------------------------------------- */

static const char font_chars[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-./'!?+";

static const UBYTE font_rows[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11}, /* A */
    {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e}, /* B */
    {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e}, /* C */
    {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e}, /* D */
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f}, /* E */
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10}, /* F */
    {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f}, /* G */
    {0x11,0x11,0x11,0x1f,0x11,0x11,0x11}, /* H */
    {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f}, /* I */
    {0x07,0x02,0x02,0x02,0x12,0x12,0x0c}, /* J */
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, /* K */
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1f}, /* L */
    {0x11,0x1b,0x15,0x15,0x11,0x11,0x11}, /* M */
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, /* N */
    {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e}, /* O */
    {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10}, /* P */
    {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d}, /* Q */
    {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11}, /* R */
    {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e}, /* S */
    {0x1f,0x04,0x04,0x04,0x04,0x04,0x04}, /* T */
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0e}, /* U */
    {0x11,0x11,0x11,0x11,0x11,0x0a,0x04}, /* V */
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0a}, /* W */
    {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11}, /* X */
    {0x11,0x11,0x0a,0x04,0x04,0x04,0x04}, /* Y */
    {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f}, /* Z */
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e}, /* 0 */
    {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e}, /* 1 */
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f}, /* 2 */
    {0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e}, /* 3 */
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02}, /* 4 */
    {0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e}, /* 5 */
    {0x0e,0x10,0x10,0x1e,0x11,0x11,0x0e}, /* 6 */
    {0x1f,0x01,0x02,0x04,0x08,0x08,0x08}, /* 7 */
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e}, /* 8 */
    {0x0e,0x11,0x11,0x0f,0x01,0x01,0x0e}, /* 9 */
    {0x00,0x04,0x04,0x00,0x04,0x04,0x00}, /* : */
    {0x00,0x00,0x00,0x1f,0x00,0x00,0x00}, /* - */
    {0x00,0x00,0x00,0x00,0x00,0x06,0x06}, /* . */
    {0x01,0x02,0x02,0x04,0x08,0x08,0x10}, /* / */
    {0x04,0x04,0x08,0x00,0x00,0x00,0x00}, /* ' */
    {0x0e,0x11,0x01,0x02,0x04,0x00,0x04}, /* ! actually ? */
    {0x04,0x04,0x04,0x04,0x00,0x00,0x04}, /* ? actually ! */
    {0x00,0x04,0x04,0x1f,0x04,0x04,0x00}  /* + */
};

/* ---- game data ----------------------------------------------------- */

typedef struct {
    int curve;
    int y;
    UBYTE scenery;
    UBYTE flags;
} TrackSegment;

typedef struct {
    int x1, y1, w1;
    int x2, y2, w2;
    int segment;
    int visible;
} ProjectedSegment;

typedef struct {
    int position;
    int speed;
    int lane_q16;
    int target_lane_q16;
    int lap;
    int color;
    int aggression;
} RaceCar;

typedef enum {
    STATE_TITLE = 0,
    STATE_COUNTDOWN,
    STATE_RACE,
    STATE_FINISHED
} GameState;

static TrackSegment track[TRACK_SEGMENTS];
static ProjectedSegment projected[DRAW_DISTANCE + 2];
static RaceCar cars[MAX_CARS];
static int track_cursor;
static int track_last_y;
static int track_length;
static int player_position;
static int player_speed;
static int player_lane_q16;
static int player_lap;
static int player_rank;
static int race_laps = DEFAULT_LAPS;
static int race_elapsed_ms;
static int lap_start_ms;
static int best_lap_ms;
static int last_lap_ms;
static int countdown_frames;
static int title_scroll;
static int finish_place;
static int game_frame;
static int demo_seconds;
static int no_audio;
static int force_rtg = 1;
static int auto_start;
static GameState game_state = STATE_TITLE;
static acgame_platform_info platform_info;

/* ---- integer helpers -------------------------------------------------- */

static int clamp_int(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static int abs_int(int value)
{
    return value < 0 ? -value : value;
}

static int ease_q16(int t)
{
    int64_t a;
    t = clamp_int(t, 0, Q16_ONE);
    a = (int64_t)t * t >> 16;
    return (int)(a * (3 * Q16_ONE - 2 * t) >> 16);
}

static int lerp_int(int a, int b, int t_q16)
{
    return a + (int)(((int64_t)(b - a) * t_q16) >> 16);
}

/* ---- track ------------------------------------------------------ */

static void add_road(int enter, int hold, int leave, int curve, int hill)
{
    int total = enter + hold + leave;
    int start_y = track_last_y;
    int end_y = start_y + hill;
    int i;
    if (total <= 0)
        return;
    for (i = 0; i < total && track_cursor < TRACK_SEGMENTS; ++i) {
        int c;
        int t = (int)(((int64_t)i << 16) / (total - 1 > 0 ? total - 1 : 1));
        if (i < enter)
            c = curve * ease_q16((int)(((int64_t)i << 16) / (enter ? enter : 1))) / Q16_ONE;
        else if (i < enter + hold)
            c = curve;
        else
            c = curve * (Q16_ONE - ease_q16((int)(((int64_t)(i - enter - hold) << 16) /
                                                   (leave ? leave : 1)))) / Q16_ONE;
        track[track_cursor].curve = c;
        track[track_cursor].y = lerp_int(start_y, end_y, ease_q16(t));
        track[track_cursor].scenery = (UBYTE)((track_cursor / 13) % 6);
        track[track_cursor].flags = 0;
        ++track_cursor;
    }
    track_last_y = end_y;
}

static void build_track(void)
{
    int remaining;
    memset(track, 0, sizeof(track));
    track_cursor = 0;
    track_last_y = 0;

    add_road(0, 70, 0, 0, 0);       /* start / grandstand */
    add_road(30, 55, 30, 2, 240);   /* paddock bend */
    add_road(20, 35, 20, -5, -120); /* red kite hairpin */
    add_road(18, 22, 18, 7, 180);   /* boing kink */
    add_road(25, 55, 25, -3, 420);  /* high moor */
    add_road(30, 70, 30, 2, -560);  /* long drop */
    add_road(12, 18, 12, -7, 0);    /* chicane one */
    add_road(12, 18, 12, 7, 0);     /* chicane two */
    add_road(0, 95, 0, 0, 120);      /* turbo straight */
    add_road(30, 60, 30, 4, 80);    /* carousel */
    add_road(20, 32, 20, -6, -260); /* lake hairpin */
    add_road(25, 45, 25, 3, 180);   /* final esses */
    add_road(16, 24, 16, -4, -120);

    remaining = TRACK_SEGMENTS - track_cursor;
    if (remaining > 0)
        add_road(remaining / 4, remaining / 2, remaining - (remaining / 4 + remaining / 2),
                 0, -track_last_y);

    while (track_cursor < TRACK_SEGMENTS) {
        track[track_cursor].curve = 0;
        track[track_cursor].y = 0;
        track[track_cursor].scenery = 0;
        track[track_cursor].flags = 0;
        ++track_cursor;
    }

    track[0].flags |= 1;
    track[1].flags |= 1;
    track_length = TRACK_SEGMENTS * SEG_LEN;
}

/* ---- indexed drawing -------------------------------------------------- */

static acgame_indexed_surface *frame_surface;

static void put_pixel(int x, int y, UBYTE color)
{
    if ((unsigned)x < GAME_W && (unsigned)y < GAME_H)
        frame_surface->pixels[(size_t)y * frame_surface->pitch + (unsigned)x] = color;
}

static void hline(int x0, int x1, int y, UBYTE color)
{
    UBYTE *dst;
    int n;
    if ((unsigned)y >= GAME_H)
        return;
    if (x0 > x1) {
        int t = x0; x0 = x1; x1 = t;
    }
    if (x1 < 0 || x0 >= GAME_W)
        return;
    if (x0 < 0) x0 = 0;
    if (x1 >= GAME_W) x1 = GAME_W - 1;
    dst = frame_surface->pixels + (size_t)y * frame_surface->pitch + (unsigned)x0;
    n = x1 - x0 + 1;
    memset(dst, color, (size_t)n);
}

static void fill_rect(int x, int y, int w, int h, UBYTE color)
{
    int yy;
    if (w <= 0 || h <= 0)
        return;
    for (yy = y; yy < y + h; ++yy)
        hline(x, x + w - 1, yy, color);
}

static void fill_quad(int far_left, int far_right, int far_y,
                      int near_left, int near_right, int near_y,
                      UBYTE color)
{
    int y, dy;
    if (near_y < far_y) {
        int t;
        t = near_y; near_y = far_y; far_y = t;
        t = near_left; near_left = far_left; far_left = t;
        t = near_right; near_right = far_right; far_right = t;
    }
    if (near_y < 0 || far_y >= GAME_H)
        return;
    dy = near_y - far_y;
    if (dy <= 0) {
        hline(far_left, far_right, far_y, color);
        return;
    }
    if (far_y < 0) far_y = 0;
    if (near_y >= GAME_H) near_y = GAME_H - 1;
    for (y = far_y; y <= near_y; ++y) {
        int t = (int)(((int64_t)(y - far_y) << 16) / dy);
        int left = lerp_int(far_left, near_left, t);
        int right = lerp_int(far_right, near_right, t);
        hline(left, right, y, color);
    }
}

static int font_index(char c)
{
    const char *p;
    if (c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 'A');
    p = strchr(font_chars, c);
    return p ? (int)(p - font_chars) : 0;
}

static void draw_char(int x, int y, char c, UBYTE color, int scale)
{
    int index = font_index(c);
    int row, col;
    if (scale < 1) scale = 1;
    for (row = 0; row < 7; ++row) {
        UBYTE bits = font_rows[index][row];
        for (col = 0; col < 5; ++col) {
            if (bits & (1U << (4 - col)))
                fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

static void draw_text(int x, int y, const char *text, UBYTE color, int scale)
{
    while (*text) {
        draw_char(x, y, *text++, color, scale);
        x += 6 * scale;
    }
}

static int text_width(const char *text, int scale)
{
    return (int)strlen(text) * 6 * scale - scale;
}

static void draw_text_center(int y, const char *text, UBYTE color, int scale)
{
    draw_text((GAME_W - text_width(text, scale)) / 2, y, text, color, scale);
}

static void set_palette(void)
{
    acgame_rgb8 *p = frame_surface->palette;
    unsigned i;
    for (i = 0; i < 256; ++i) {
        p[i].r = 0;
        p[i].g = 0;
        p[i].b = 0;
    }
#define RGB(index, rr, gg, bb) do { p[index].r=(rr); p[index].g=(gg); p[index].b=(bb); } while (0)
    RGB(COL_BLACK, 0, 0, 0);
    RGB(COL_SKY0, 14, 24, 52);
    RGB(COL_SKY1, 34, 74, 126);
    RGB(COL_SKY2, 108, 162, 194);
    RGB(COL_CLOUD, 204, 218, 220);
    RGB(COL_MOUNTAIN0, 36, 50, 68);
    RGB(COL_MOUNTAIN1, 58, 72, 86);
    RGB(COL_GRASS0, 30, 92, 52);
    RGB(COL_GRASS1, 38, 112, 58);
    RGB(COL_GRASS2, 56, 128, 66);
    RGB(COL_ROAD0, 54, 58, 64);
    RGB(COL_ROAD1, 61, 64, 70);
    RGB(COL_RUM