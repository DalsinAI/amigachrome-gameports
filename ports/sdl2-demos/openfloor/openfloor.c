/*
 * OpenFloor - infinite perspective floor, copper bars and Boing Ball.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>

#define FLOOR_W 320
#define FLOOR_H 240
#define BALL_SIZE 64

static SDL_Texture *make_boing_ball(SDL_Renderer *renderer)
{
    Uint32 pixels[BALL_SIZE * BALL_SIZE];
    SDL_Texture *texture;
    float c = (BALL_SIZE - 1) * 0.5f;
    float r = BALL_SIZE * 0.47f;
    int x;
    int y;

    for (y = 0; y < BALL_SIZE; ++y) {
        for (x = 0; x < BALL_SIZE; ++x) {
            float nx = (x - c) / r;
            float ny = (y - c) / r;
            float rr = nx * nx + ny * ny;
            Uint32 out = 0;
            if (rr <= 1.0f) {
                float nz = sqrtf(1.0f - rr);
                float longitude = atan2f(nx, nz) / 3.14159265f;
                float latitude = asinf(ny) / 3.14159265f;
                int cx = (int)floorf((longitude + 1.0f) * 6.0f);
                int cy = (int)floorf((latitude + 0.5f) * 8.0f);
                int white = (cx ^ cy) & 1;
                float light = 0.34f + 0.66f * (0.30f * nx - 0.30f * ny + 0.90f * nz);
                int br = white ? 245 : 235;
                int bg = white ? 245 : 30;
                int bb = white ? 245 : 30;
                int pr = (int)(br * light);
                int pg = (int)(bg * light);
                int pb = (int)(bb * light);
                if (pr > 255) pr = 255;
                if (pg > 255) pg = 255;
                if (pb > 255) pb = 255;
                out = demo_argb((Uint8)pr, (Uint8)pg, (Uint8)pb);
            }
            pixels[y * BALL_SIZE + x] = out;
        }
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, BALL_SIZE, BALL_SIZE);
    if (!texture) return NULL;
    SDL_UpdateTexture(texture, NULL, pixels, BALL_SIZE * (int)sizeof(Uint32));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

static void draw_sky(SDL_Renderer *renderer)
{
    int y;
    for (y = 0; y < 132; ++y) {
        int r = 3 + y * 14 / 132;
        int g = 5 + y * 28 / 132;
        int b = 20 + y * 76 / 132;
        SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
        SDL_RenderDrawLine(renderer, 0, y, FLOOR_W - 1, y);
    }
}

static void draw_copper_bars(SDL_Renderer *renderer, float t, int palette)
{
    int bar;
    static const Uint8 colours[3][3][3] = {
        {{255,40,25},{255,170,25},{255,245,120}},
        {{35,120,255},{30,235,230},{190,255,255}},
        {{210,45,255},{255,45,130},{255,210,250}}
    };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (bar = 0; bar < 3; ++bar) {
        int center = 36 + bar * 26 + (int)(sinf(t * (0.8f + bar * 0.17f) + bar * 1.8f) * 22.0f);
        int row;
        for (row = -8; row <= 8; ++row) {
            int fade = 255 - abs(row) * 28;
            int r = colours[palette][bar][0] * fade / 255;
            int g = colours[palette][bar][1] * fade / 255;
            int b = colours[palette][bar][2] * fade / 255;
            if (fade < 0) fade = 0;
            SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, 150);
            SDL_RenderDrawLine(renderer, 0, center + row, FLOOR_W - 1, center + row);
        }
    }
}

static void draw_floor(SDL_Renderer *renderer, float t)
{
    const int horizon = 132;
    int i;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 8, 10, 20, 255);
    {
        SDL_Rect lower = {0, horizon, FLOOR_W, FLOOR_H - horizon};
        SDL_RenderFillRect(renderer, &lower);
    }

    for (i = 0; i < 24; ++i) {
        float q = fmodf(i / 24.0f + t * 0.22f, 1.0f);
        float p = q * q;
        int y = horizon + (int)(p * (FLOOR_H - horizon));
        int c = 55 + (int)(q * 180.0f);
        SDL_SetRenderDrawColor(renderer, (Uint8)(c / 3), (Uint8)c, 255, 185);
        SDL_RenderDrawLine(renderer, 0, y, FLOOR_W - 1, y);
    }

    for (i = -16; i <= 16; ++i) {
        int x0 = FLOOR_W / 2 + i * 5;
        int x1 = FLOOR_W / 2 + i * 27 + (int)(sinf(t * 0.55f) * 18.0f);
        SDL_SetRenderDrawColor(renderer, 35, 150, 245, 150);
        SDL_RenderDrawLine(renderer, x0, horizon, x1, FLOOR_H - 1);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (i = 0; i < 5; ++i) {
        int y = horizon + 6 + i * 3;
        SDL_SetRenderDrawColor(renderer, 35, 90, 220, (Uint8)(110 - i * 18));
        SDL_RenderDrawLine(renderer, 0, y, FLOOR_W - 1, y);
    }
}

static void draw_shadow(SDL_Renderer *renderer, int center_x, int center_y, float scale)
{
    int row;
    int radius = (int)(28.0f * scale);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (row = -6; row <= 6; ++row) {
        float q = row / 6.0f;
        int half = (int)(radius * sqrtf(1.0f - q * q));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)(70 - abs(row) * 6));
        SDL_RenderDrawLine(renderer, center_x - half, center_y + row,
                           center_x + half, center_y + row);
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *ball;
    SDL_Event event;
    Uint32 start;
    float frozen_t = 0.0f;
    float speed = 1.0f;
    int running = 1;
    int paused = 0;
    int palette = 0;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenFloor", FLOOR_W, FLOOR_H, 2)) return 1;
    ball = make_boing_ball(app.renderer);
    if (!ball) {
        fprintf(stderr, "OpenFloor: ball texture failed: %s\n", SDL_GetError());
        demo_shutdown(&app);
        return 2;
    }
    start = SDL_GetTicks();

    while (running) {
        float raw_t = paused ? frozen_t : (SDL_GetTicks() - start) * 0.001f;
        float t = raw_t * speed;
        float bounce = fabsf(sinf(t * 2.35f));
        float scale = 0.72f + bounce * 0.28f;
        int ball_w = (int)(BALL_SIZE * scale);
        int ball_h = ball_w;
        int ball_x = FLOOR_W / 2 - ball_w / 2 + (int)(sinf(t * 0.72f) * 70.0f);
        int ball_y = 144 - ball_h - (int)(bounce * 82.0f);
        SDL_Rect dst = {ball_x, ball_y, ball_w, ball_h};
        char mode[96];

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) continue;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        if (paused) frozen_t = raw_t;
                        else start = SDL_GetTicks() - (Uint32)(frozen_t * 1000.0f);
                        break;
                    case SDLK_c:
                        palette = (palette + 1) % 3;
                        break;
                    case SDLK_UP:
                    case SDLK_RIGHT:
                        if (speed < 3.0f) speed += 0.25f;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        if (speed > 0.25f) speed -= 0.25f;
                        break;
                    default:
                        break;
                }
            }
        }

        draw_sky(app.renderer);
        draw_copper_bars(app.renderer, t, palette);
        draw_floor(app.renderer, t);
        draw_shadow(app.renderer, ball_x + ball_w / 2, 148, scale);
        SDL_RenderCopyEx(app.renderer, ball, NULL, &dst,
                         (double)t * 70.0, NULL, SDL_FLIP_NONE);
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "%s speed %.2f palette %d - arrows/C/Space/F/Esc",
                 paused ? "paused" : "running", (double)speed, palette + 1);
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(ball);
    demo_shutdown(&app);
    return 0;
}
