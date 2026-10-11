/*
 * OpenFire - compact Doom-style fire for OpenUp SDL2/OpenGPU.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <stdlib.h>

#define FIRE_W 160
#define FIRE_H 100
#define FIRE_LEVELS 37

static Uint32 rng_state = 0x4f50454eu;

static Uint32 fast_rand(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static void make_palette(Uint32 palette[FIRE_LEVELS], int theme)
{
    int i;
    for (i = 0; i < FIRE_LEVELS; ++i) {
        int r = 0;
        int g = 0;
        int b = 0;
        int x = i * 255 / (FIRE_LEVELS - 1);

        if (theme == 0) {
            if (x < 96) {
                r = x * 255 / 96;
            } else if (x < 192) {
                r = 255;
                g = (x - 96) * 255 / 96;
            } else {
                r = 255;
                g = 255;
                b = (x - 192) * 255 / 63;
            }
        } else if (theme == 1) {
            if (x < 128) {
                b = x * 2;
                r = x / 2;
            } else {
                b = 255;
                r = 64 + (x - 128) * 191 / 127;
                g = (x - 128) * 255 / 127;
            }
        } else {
            if (x < 128) {
                g = x * 2;
                b = x;
            } else {
                g = 255;
                b = 128 + (x - 128);
                r = (x - 128) * 255 / 127;
            }
        }
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        palette[i] = demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
    }
}

static void update_fire(Uint8 *heat, int intensity)
{
    int x;
    int y;

    for (x = 0; x < FIRE_W; ++x) {
        Uint32 r = fast_rand();
        heat[(FIRE_H - 1) * FIRE_W + x] =
            (Uint8)((r & 7u) < (Uint32)intensity ? FIRE_LEVELS - 1 : FIRE_LEVELS - 5);
    }

    for (y = 1; y < FIRE_H; ++y) {
        for (x = 0; x < FIRE_W; ++x) {
            int src = y * FIRE_W + x;
            int drift = (int)(fast_rand() & 3u) - 1;
            int dst_x = x + drift;
            int value = heat[src] - (int)(fast_rand() & 1u);
            if (dst_x < 0) dst_x += FIRE_W;
            if (dst_x >= FIRE_W) dst_x -= FIRE_W;
            if (value < 0) value = 0;
            heat[(y - 1) * FIRE_W + dst_x] = (Uint8)value;
        }
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *texture = NULL;
    Uint8 *heat = NULL;
    Uint32 *pixels = NULL;
    Uint32 palette[FIRE_LEVELS];
    int running = 1;
    int paused = 0;
    int theme = 0;
    int intensity = 6;
    SDL_Event event;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenFire", FIRE_W, FIRE_H, 4)) {
        return 1;
    }

    heat = (Uint8 *)calloc((size_t)FIRE_W * FIRE_H, 1);
    pixels = (Uint32 *)malloc((size_t)FIRE_W * FIRE_H * sizeof(*pixels));
    texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, FIRE_W, FIRE_H);
    if (!heat || !pixels || !texture) {
        fprintf(stderr, "OpenFire: allocation failed: %s\n", SDL_GetError());
        free(heat);
        free(pixels);
        if (texture) SDL_DestroyTexture(texture);
        demo_shutdown(&app);
        return 2;
    }

    make_palette(palette, theme);
    while (running) {
        int i;
        char mode[96];

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_c:
                        theme = (theme + 1) % 3;
                        make_palette(palette, theme);
                        break;
                    case SDLK_UP:
                        if (intensity < 7) ++intensity;
                        break;
                    case SDLK_DOWN:
                        if (intensity > 1) --intensity;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) {
            update_fire(heat, intensity);
        }
        for (i = 0; i < FIRE_W * FIRE_H; ++i) {
            int level = heat[i];
            if (level < 0) level = 0;
            if (level >= FIRE_LEVELS) level = FIRE_LEVELS - 1;
            pixels[i] = palette[level];
        }

        SDL_UpdateTexture(texture, NULL, pixels, FIRE_W * (int)sizeof(Uint32));
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, texture, NULL, NULL);
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "%s palette %d, heat %d - C/Up/Down/Space/F/Esc",
                 paused ? "paused" : "running", theme + 1, intensity);
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(texture);
    free(pixels);
    free(heat);
    demo_shutdown(&app);
    return 0;
}
