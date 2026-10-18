/*
 * OpenTunnel - polar tunnel, OCS-style dithering and Boing Ball badge.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdlib.h>

#define TUNNEL_W 160
#define TUNNEL_H 120
#define BALL_SIZE 40

static const Uint8 bayer4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

static SDL_Texture *make_ball(SDL_Renderer *renderer)
{
    Uint32 pixels[BALL_SIZE * BALL_SIZE];
    SDL_Texture *texture;
    int x;
    int y;
    float radius = BALL_SIZE * 0.46f;
    float center = (BALL_SIZE - 1) * 0.5f;

    for (y = 0; y < BALL_SIZE; ++y) {
        for (x = 0; x < BALL_SIZE; ++x) {
            float dx = (x - center) / radius;
            float dy = (y - center) / radius;
            float rr = dx * dx + dy * dy;
            Uint32 colour = 0x00000000u;
            if (rr <= 1.0f) {
                float z = sqrtf(1.0f - rr);
                float u = atan2f(dx, z) * 2.5f;
                float v = asinf(dy) * 2.5f;
                int check = (((int)floorf(u + 8.0f)) ^ ((int)floorf(v + 8.0f))) & 1;
                float light = 0.38f + 0.62f * (0.25f * dx - 0.35f * dy + 0.90f * z);
                int base_r = 238;
                int base_g = check ? 238 : 28;
                int base_b = check ? 238 : 28;
                int r = (int)(base_r * light);
                int g = (int)(base_g * light);
                int b = (int)(base_b * light);
                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;
                colour = demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
            }
            pixels[y * BALL_SIZE + x] = colour;
        }
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, BALL_SIZE, BALL_SIZE);
    if (!texture) {
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, NULL, pixels, BALL_SIZE * (int)sizeof(Uint32));
    return texture;
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *tunnel = NULL;
    SDL_Texture *ball = NULL;
    Uint16 *distance = NULL;
    Uint8 *angle = NULL;
    Uint32 *pixels = NULL;
    int running = 1;
    int speed = 2;
    int paused = 0;
    Uint32 frame = 0;
    SDL_Event event;
    int x;
    int y;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenTunnel", TUNNEL_W, TUNNEL_H, 4)) {
        return 1;
    }

    distance = (Uint16 *)malloc((size_t)TUNNEL_W * TUNNEL_H * sizeof(*distance));
    angle = (Uint8 *)malloc((size_t)TUNNEL_W * TUNNEL_H * sizeof(*angle));
    pixels = (Uint32 *)malloc((size_t)TUNNEL_W * TUNNEL_H * sizeof(*pixels));
    tunnel = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING, TUNNEL_W, TUNNEL_H);
    ball = make_ball(app.renderer);
    if (!distance || !angle || !pixels || !tunnel || !ball) {
        fprintf(stderr, "OpenTunnel: allocation failed: %s\n", SDL_GetError());
        if (ball) SDL_DestroyTexture(ball);
        if (tunnel) SDL_DestroyTexture(tunnel);
        free(pixels);
        free(angle);
        free(distance);
        demo_shutdown(&app);
        return 2;
    }

    for (y = 0; y < TUNNEL_H; ++y) {
        for (x = 0; x < TUNNEL_W; ++x) {
            float dx = (float)x - TUNNEL_W * 0.5f;
            float dy = (float)y - TUNNEL_H * 0.5f;
            float d = sqrtf(dx * dx + dy * dy);
            float a = atan2f(dy, dx);
            int i = y * TUNNEL_W + x;
            if (d < 1.0f) d = 1.0f;
            distance[i] = (Uint16)(4096.0f / d);
            angle[i] = (Uint8)((a + 3.14159265f) * (255.0f / 6.28318530f));
        }
    }

    while (running) {
        char mode[96];
        int i;
        int phase;

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_UP:
                    case SDLK_RIGHT:
                        if (speed < 8) ++speed;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        if (speed > -8) --speed;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) {
            frame += (Uint32)(speed >= 0 ? speed : -speed);
        }
        phase = speed < 0 ? -(int)frame : (int)frame;

        for (i = 0; i < TUNNEL_W * TUNNEL_H; ++i) {
            int px = i % TUNNEL_W;
            int py = i / TUNNEL_W;
            int u = (int)distance[i] + phase * 2;
            int v = (int)angle[i] + phase;
            int pattern = (((u >> 4) ^ (v >> 4)) & 1);
            int rings = ((u >> 3) + (v >> 5)) & 15;
            int dither = bayer4[(py & 3) * 4 + (px & 3)];
            int shade = (int)distance[i] / 18;
            int r;
            int g;
            int b;

            if (shade > 180) shade = 180;
            if (pattern) {
                r = 35 + shade + rings * 3;
                g = 18 + shade / 3;
                b = 90 + shade / 2;
            } else {
                r = 10 + shade / 4;
                g = 55 + shade + rings * 2;
                b = 85 + shade;
            }
            r += dither * 2;
            g += dither;
            b += dither * 2;
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            pixels[i] = demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
        }

        SDL_UpdateTexture(tunnel, NULL, pixels, TUNNEL_W * (int)sizeof(Uint32));
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, tunnel, NULL, NULL);
        {
            float t = (float)phase * 0.035f;
            SDL_Rect dst;
            dst.w = BALL_SIZE;
            dst.h = BALL_SIZE;
            dst.x = TUNNEL_W / 2 - BALL_SIZE / 2 + (int)(sinf(t * 0.7f) * 24.0f);
            dst.y = TUNNEL_H / 2 - BALL_SIZE / 2 + (int)(cosf(t * 0.9f) * 18.0f);
            SDL_RenderCopyEx(app.renderer, ball, NULL, &dst,
                             (double)phase * 1.7, NULL, SDL_FLIP_NONE);
        }
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "%s speed %d - arrows/Space/F/Esc",
                 paused ? "paused" : "running", speed);
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(ball);
    SDL_DestroyTexture(tunnel);
    free(pixels);
    free(angle);
    free(distance);
    demo_shutdown(&app);
    return 0;
}
