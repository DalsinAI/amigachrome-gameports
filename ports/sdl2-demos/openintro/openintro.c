/*
 * OpenIntro - star trails, fire, copper scroller and CRT scanlines.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define INTRO_W 320
#define INTRO_H 240
#define FIRE_W 160
#define FIRE_H 56
#define STAR_COUNT 72

typedef struct Glyph {
    char ch;
    Uint8 row[7];
} Glyph;

typedef struct Star {
    float x, y, z, ox, oy;
} Star;

static const Glyph glyphs[] = {
    {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
    {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
    {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,14}}, {'H',{17,17,17,31,17,17,17}},
    {'I',{31,4,4,4,4,4,31}},      {'J',{7,2,2,2,18,18,12}},
    {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
    {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
    {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}},   {'T',{31,4,4,4,4,4,4}},
    {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
    {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}},     {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
    {'2',{14,17,1,2,4,8,31}},     {'3',{30,1,1,14,1,1,30}},
    {'4',{2,6,10,18,31,2,2}},     {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
    {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
    {'-',{0,0,0,31,0,0,0}},       {' ',{0,0,0,0,0,0,0}}
};

static Uint32 rng_state = 0x494e5452u;

static Uint32 quick_rand(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state;
}

static const Uint8 *glyph_for(char ch)
{
    size_t i;
    for (i = 0; i < sizeof(glyphs) / sizeof(glyphs[0]); ++i) {
        if (glyphs[i].ch == ch) return glyphs[i].row;
    }
    return glyphs[sizeof(glyphs) / sizeof(glyphs[0]) - 1].row;
}

static void reset_star(Star *s, int fresh)
{
    s->x = ((int)(quick_rand() & 1023u) - 512) / 512.0f;
    s->y = ((int)(quick_rand() & 1023u) - 512) / 680.0f;
    s->z = fresh ? 0.08f + (quick_rand() & 1023u) / 1100.0f : 1.0f;
    s->ox = INTRO_W * 0.5f;
    s->oy = INTRO_H * 0.43f;
}

static void draw_stars(SDL_Renderer *renderer, Star stars[STAR_COUNT], int paused)
{
    int i;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (i = 0; i < STAR_COUNT; ++i) {
        Star *s = &stars[i];
        float x;
        float y;
        int c;
        if (!paused) s->z -= 0.010f;
        if (s->z < 0.035f) reset_star(s, 0);
        x = INTRO_W * 0.5f + s->x * 145.0f / s->z;
        y = INTRO_H * 0.43f + s->y * 100.0f / s->z;
        if (x < 0 || y < 0 || x >= INTRO_W || y >= INTRO_H) {
            reset_star(s, 0);
            continue;
        }
        c = 90 + (int)((1.0f - s->z) * 165.0f);
        if (c > 255) c = 255;
        SDL_SetRenderDrawColor(renderer, (Uint8)c, (Uint8)(c * 3 / 4), 255, 220);
        SDL_RenderDrawLine(renderer, (int)s->ox, (int)s->oy, (int)x, (int)y);
        s->ox = x;
        s->oy = y;
    }
}

static void update_fire(Uint8 *heat, Uint32 *pixels)
{
    static const Uint32 palette[16] = {
        0xff000000,0xff170016,0xff350020,0xff550016,
        0xff750008,0xff980000,0xffbd1700,0xffdc3500,
        0xffff5700,0xffff7b00,0xffffa300,0xffffc52d,
        0xffffdc67,0xffffed9c,0xfffff7cd,0xffffffff
    };
    int x, y;
    for (x = 0; x < FIRE_W; ++x) {
        heat[(FIRE_H - 1) * FIRE_W + x] = (quick_rand() & 7u) ? 15 : 10;
    }
    for (y = 1; y < FIRE_H; ++y) {
        for (x = 0; x < FIRE_W; ++x) {
            int src = y * FIRE_W + x;
            int nx = x + (int)(quick_rand() % 3u) - 1;
            int v = heat[src] - (int)(quick_rand() & 1u);
            if (nx < 0) nx += FIRE_W;
            if (nx >= FIRE_W) nx -= FIRE_W;
            if (v < 0) v = 0;
            heat[(y - 1) * FIRE_W + nx] = (Uint8)v;
        }
    }
    for (y = 0; y < FIRE_H; ++y) {
        for (x = 0; x < FIRE_W; ++x) {
            pixels[y * FIRE_W + x] = palette[heat[y * FIRE_W + x] & 15];
        }
    }
}

static void draw_scroller(SDL_Renderer *renderer, float t)
{
    static const char text[] = "OPENUP SDL2 DEMOS - AMIGA POWER - THE FUTURE IS RETRO - ";
    const int count = (int)strlen(text);
    const float advance = 17.0f;
    const float total = advance * count;
    float start = INTRO_W - fmodf(t * 54.0f, total);
    int copy;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (copy = 0; copy < 2; ++copy) {
        int i;
        for (i = 0; i < count; ++i) {
            float base_x = start + copy * total + i * advance;
            float phase = base_x * 0.033f + t * 1.7f;
            float depth = 0.70f + 0.30f * (0.5f + 0.5f * cosf(phase));
            int scale = depth > 0.86f ? 3 : 2;
            int base_y = 103 + (int)(sinf(phase) * 30.0f);
            const Uint8 *rows = glyph_for(text[i]);
            int row;
            int r = (int)(140 + 115 * sinf(phase));
            int g = (int)(140 + 115 * sinf(phase + 2.094f));
            int b = (int)(140 + 115 * sinf(phase + 4.188f));
            if (base_x < -20 || base_x > INTRO_W + 20) continue;
            for (row = 0; row < 7; ++row) {
                int col;
                for (col = 0; col < 5; ++col) {
                    if (rows[row] & (1u << (4 - col))) {
                        SDL_Rect rect;
                        rect.x = (int)base_x + col * scale;
                        rect.y = base_y + row * scale;
                        rect.w = scale;
                        rect.h = scale;
                        SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, 245);
                        SDL_RenderFillRect(renderer, &rect);
                        rect.y += 20;
                        SDL_SetRenderDrawColor(renderer, (Uint8)(r / 3), (Uint8)(g / 3), (Uint8)(b / 2), 80);
                        SDL_RenderFillRect(renderer, &rect);
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *fire_texture;
    Uint8 *heat;
    Uint32 *fire_pixels;
    Star stars[STAR_COUNT];
    SDL_Event event;
    Uint32 start;
    float frozen_t = 0.0f;
    int running = 1;
    int paused = 0;
    int i;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenIntro", INTRO_W, INTRO_H, 2)) return 1;
    heat = (Uint8 *)calloc((size_t)FIRE_W * FIRE_H, 1);
    fire_pixels = (Uint32 *)malloc((size_t)FIRE_W * FIRE_H * sizeof(*fire_pixels));
    fire_texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, FIRE_W, FIRE_H);
    if (!heat || !fire_pixels || !fire_texture) {
        fprintf(stderr, "OpenIntro: allocation failed: %s\n", SDL_GetError());
        free(heat); free(fire_pixels);
        if (fire_texture) SDL_DestroyTexture(fire_texture);
        demo_shutdown(&app);
        return 2;
    }
    SDL_SetTextureBlendMode(fire_texture, SDL_BLENDMODE_ADD);
    for (i = 0; i < STAR_COUNT; ++i) reset_star(&stars[i], 1);
    start = SDL_GetTicks();

    while (running) {
        float t = paused ? frozen_t : (SDL_GetTicks() - start) * 0.001f;
        SDL_Rect fire_dst = {0, INTRO_H - 100, INTRO_W, 100};
        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) continue;
            if (event.type == SDL_KEYDOWN && !event.key.repeat &&
                event.key.keysym.sym == SDLK_SPACE) {
                paused = !paused;
                if (paused) frozen_t = t;
                else start = SDL_GetTicks() - (Uint32)(frozen_t * 1000.0f);
            }
        }

        if (!paused) update_fire(heat, fire_pixels);
        SDL_UpdateTexture(fire_texture, NULL, fire_pixels, FIRE_W * (int)sizeof(Uint32));
        SDL_SetRenderDrawColor(app.renderer, 2, 2, 14, 255);
        SDL_RenderClear(app.renderer);
        draw_stars(app.renderer, stars, paused);
        SDL_RenderCopy(app.renderer, fire_texture, NULL, &fire_dst);
        draw_scroller(app.renderer, t);

        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 70);
        for (i = 0; i < INTRO_H; i += 2) {
            SDL_RenderDrawLine(app.renderer, 0, i, INTRO_W - 1, i);
        }
        SDL_RenderPresent(app.renderer);
        demo_title_tick(&app, paused ? "paused - Space/F/Esc" : "star trails + fire + copper scroller - Space/F/Esc");
    }

    SDL_DestroyTexture(fire_texture);
    free(fire_pixels);
    free(heat);
    demo_shutdown(&app);
    return 0;
}
