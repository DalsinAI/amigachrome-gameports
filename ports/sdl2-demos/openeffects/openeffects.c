/*
 * OpenEffects - plasma, metaballs, rotozoom and deformation in SDL2.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdlib.h>

#define FX_W 160
#define FX_H 120
#define LUT_SIZE 1024
#define TEX_SIZE 64

static int sin_lut[LUT_SIZE];
static Uint32 colour_lut[256];
static Uint32 source_tex[TEX_SIZE * TEX_SIZE];

static void prepare_tables(void)
{
    int i;
    int x;
    int y;
    for (i = 0; i < LUT_SIZE; ++i) {
        sin_lut[i] = (int)(sinf((float)i * 6.283185307f / LUT_SIZE) * 127.0f + 128.0f);
    }
    for (i = 0; i < 256; ++i) {
        float p = (float)i * 6.283185307f / 256.0f;
        int r = (int)(128.0f + 127.0f * sinf(p));
        int g = (int)(128.0f + 127.0f * sinf(p + 2.094395102f));
        int b = (int)(128.0f + 127.0f * sinf(p + 4.188790205f));
        colour_lut[i] = demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
    }
    for (y = 0; y < TEX_SIZE; ++y) {
        for (x = 0; x < TEX_SIZE; ++x) {
            int check = ((x >> 3) ^ (y >> 3)) & 1;
            int rings = ((x - 32) * (x - 32) + (y - 32) * (y - 32)) >> 6;
            if ((rings & 1) != 0) check = !check;
            source_tex[y * TEX_SIZE + x] = check
                ? demo_argb(245, 45, 180)
                : demo_argb(30, 220, 235);
        }
    }
}

static void render_plasma(Uint32 *pixels, Uint32 tick)
{
    int x;
    int y;
    int t = (int)(tick & (LUT_SIZE - 1));
    for (y = 0; y < FX_H; ++y) {
        for (x = 0; x < FX_W; ++x) {
            int a = sin_lut[(x * 9 + t * 2) & (LUT_SIZE - 1)];
            int b = sin_lut[(y * 11 - t * 3) & (LUT_SIZE - 1)];
            int c = sin_lut[((x + y) * 6 + t * 4) & (LUT_SIZE - 1)];
            int dx = x - FX_W / 2;
            int dy = y - FX_H / 2;
            int d = sin_lut[((dx * dx + dy * dy) / 5 + t * 5) & (LUT_SIZE - 1)];
            pixels[y * FX_W + x] = colour_lut[(a + b + c + d) >> 2];
        }
    }
}

static void render_metaballs(Uint32 *pixels, Uint32 tick)
{
    int cx[4];
    int cy[4];
    int x;
    int y;
    int i;
    int t = (int)tick;

    cx[0] = FX_W / 2 + (sin_lut[(t * 5) & 1023] - 128) * 48 / 128;
    cy[0] = FX_H / 2 + (sin_lut[(t * 7 + 180) & 1023] - 128) * 34 / 128;
    cx[1] = FX_W / 2 + (sin_lut[(t * 3 + 320) & 1023] - 128) * 58 / 128;
    cy[1] = FX_H / 2 + (sin_lut[(t * 6 + 520) & 1023] - 128) * 42 / 128;
    cx[2] = FX_W / 2 + (sin_lut[(t * 8 + 700) & 1023] - 128) * 40 / 128;
    cy[2] = FX_H / 2 + (sin_lut[(t * 4 + 80) & 1023] - 128) * 46 / 128;
    cx[3] = FX_W / 2 + (sin_lut[(t * 2 + 900) & 1023] - 128) * 62 / 128;
    cy[3] = FX_H / 2 + (sin_lut[(t * 9 + 240) & 1023] - 128) * 30 / 128;

    for (y = 0; y < FX_H; ++y) {
        for (x = 0; x < FX_W; ++x) {
            int field = 0;
            for (i = 0; i < 4; ++i) {
                int dx = x - cx[i];
                int dy = y - cy[i];
                field += 2600 / (dx * dx + dy * dy + 12);
            }
            if (field > 255) field = 255;
            if (field < 34) {
                pixels[y * FX_W + x] = demo_argb(2, 4, 16);
            } else {
                pixels[y * FX_W + x] = colour_lut[(field + t * 2) & 255];
            }
        }
    }
}

static void render_rotozoom(Uint32 *pixels, Uint32 tick)
{
    float angle = (float)tick * 0.018f;
    float zoom = 0.62f + 0.26f * sinf((float)tick * 0.027f);
    float cs = cosf(angle) * zoom;
    float sn = sinf(angle) * zoom;
    int x;
    int y;

    for (y = 0; y < FX_H; ++y) {
        int dy = y - FX_H / 2;
        for (x = 0; x < FX_W; ++x) {
            int dx = x - FX_W / 2;
            int u = ((int)(dx * cs - dy * sn) + (int)tick) & (TEX_SIZE - 1);
            int v = ((int)(dx * sn + dy * cs) + (int)(tick * 2)) & (TEX_SIZE - 1);
            Uint32 c = source_tex[v * TEX_SIZE + u];
            int shade = 72 + (sin_lut[(x * 4 + y * 5 + tick * 3) & 1023] >> 1);
            int r = (int)((c >> 16) & 255u) * shade / 200;
            int g = (int)((c >> 8) & 255u) * shade / 200;
            int b = (int)(c & 255u) * shade / 200;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            pixels[y * FX_W + x] = demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
        }
    }
}

static void render_deform(Uint32 *pixels, Uint32 tick)
{
    int x;
    int y;
    for (y = 0; y < FX_H; ++y) {
        int yoff = (sin_lut[(y * 13 + tick * 5) & 1023] - 128) / 8;
        for (x = 0; x < FX_W; ++x) {
            int xoff = (sin_lut[(x * 15 - tick * 4) & 1023] - 128) / 8;
            int u = (x + yoff + (int)tick) & (TEX_SIZE - 1);
            int v = (y + xoff + (int)(tick >> 1)) & (TEX_SIZE - 1);
            Uint32 c = source_tex[v * TEX_SIZE + u];
            if (((x + y + (int)tick) & 31) < 2) {
                c = demo_argb(255, 255, 255);
            }
            pixels[y * FX_W + x] = c;
        }
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *texture;
    Uint32 *pixels;
    SDL_Event event;
    Uint32 tick = 0;
    int running = 1;
    int paused = 0;
    int effect = 0;
    static const char *names[4] = {"plasma", "metaballs", "rotozoom", "deform"};
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenEffects", FX_W, FX_H, 4)) {
        return 1;
    }
    prepare_tables();
    pixels = (Uint32 *)malloc((size_t)FX_W * FX_H * sizeof(*pixels));
    texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, FX_W, FX_H);
    if (!pixels || !texture) {
        fprintf(stderr, "OpenEffects: allocation failed: %s\n", SDL_GetError());
        free(pixels);
        if (texture) SDL_DestroyTexture(texture);
        demo_shutdown(&app);
        return 2;
    }

    while (running) {
        char mode[96];
        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                    case SDLK_RIGHT:
                        effect = (effect + 1) & 3;
                        break;
                    case SDLK_LEFT:
                        effect = (effect + 3) & 3;
                        break;
                    case SDLK_p:
                        paused = !paused;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) ++tick;
        switch (effect) {
            case 0: render_plasma(pixels, tick); break;
            case 1: render_metaballs(pixels, tick); break;
            case 2: render_rotozoom(pixels, tick); break;
            default: render_deform(pixels, tick); break;
        }

        SDL_UpdateTexture(texture, NULL, pixels, FX_W * (int)sizeof(Uint32));
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, texture, NULL, NULL);
        SDL_RenderPresent(app.renderer);
        snprintf(mode, sizeof(mode), "%s%s - arrows/Space/P/F/Esc",
                 names[effect], paused ? " paused" : "");
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(texture);
    free(pixels);
    demo_shutdown(&app);
    return 0;
}
