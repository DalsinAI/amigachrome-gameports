/*
 * OpenHypno - line tunnel, starfield and wireframe cube floor.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdlib.h>

#define HYPNO_W 320
#define HYPNO_H 240
#define STARS 96
#define RINGS 30
#define SEGMENTS 20

typedef struct Star {
    float x;
    float y;
    float z;
    float old_x;
    float old_y;
} Star;

static Uint32 rng_state = 0x4859504eu;

static float frand_range(float low, float high)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return low + (high - low) * (float)(rng_state & 0xffffu) / 65535.0f;
}

static void reset_star(Star *s, int fresh)
{
    s->x = frand_range(-1.0f, 1.0f);
    s->y = frand_range(-0.78f, 0.78f);
    s->z = fresh ? frand_range(0.08f, 1.0f) : 1.0f;
    s->old_x = HYPNO_W * 0.5f;
    s->old_y = HYPNO_H * 0.42f;
}

static void draw_starfield(SDL_Renderer *renderer, Star stars[STARS], float speed,
                           float center_x, float center_y)
{
    int i;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (i = 0; i < STARS; ++i) {
        Star *s = &stars[i];
        float sx;
        float sy;
        int bright;
        s->z -= speed;
        if (s->z <= 0.035f) reset_star(s, 0);
        sx = center_x + s->x * 150.0f / s->z;
        sy = center_y + s->y * 115.0f / s->z;
        if (sx < 0.0f || sy < 0.0f || sx >= HYPNO_W || sy >= HYPNO_H) {
            reset_star(s, 0);
            continue;
        }
        bright = 80 + (int)((1.0f - s->z) * 175.0f);
        if (bright > 255) bright = 255;
        SDL_SetRenderDrawColor(renderer, (Uint8)bright, (Uint8)bright,
                               (Uint8)(bright > 160 ? 255 : bright + 60), 210);
        SDL_RenderDrawLine(renderer, (int)s->old_x, (int)s->old_y, (int)sx, (int)sy);
        s->old_x = sx;
        s->old_y = sy;
    }
}

static void draw_tunnel(SDL_Renderer *renderer, float t, int direction)
{
    int ring;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (ring = RINGS - 1; ring >= 0; --ring) {
        SDL_Point points[SEGMENTS + 1];
        float depth = (float)ring / (float)RINGS;
        float z = 0.14f + depth * 1.9f;
        float radius = 9.0f + 148.0f * depth;
        float wobble_x = sinf(t * 0.63f + z * 2.7f) * 37.0f * depth;
        float wobble_y = cosf(t * 0.49f + z * 3.1f) * 25.0f * depth;
        float twist = t * (0.65f * direction) + z * 2.2f;
        int seg;
        int r = (int)(125.0f + 120.0f * sinf(z * 4.0f + t));
        int g = (int)(125.0f + 120.0f * sinf(z * 4.0f + t + 2.094f));
        int b = (int)(125.0f + 120.0f * sinf(z * 4.0f + t + 4.188f));
        int alpha = 55 + (int)(depth * 190.0f);
        SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)alpha);
        for (seg = 0; seg <= SEGMENTS; ++seg) {
            float a = twist + (float)seg * 6.283185307f / SEGMENTS;
            float squash = 0.70f + 0.16f * sinf(t * 0.4f + depth * 5.0f);
            points[seg].x = (int)(HYPNO_W * 0.5f + wobble_x + cosf(a) * radius);
            points[seg].y = (int)(HYPNO_H * 0.42f + wobble_y + sinf(a) * radius * squash);
        }
        SDL_RenderDrawLines(renderer, points, SEGMENTS + 1);
    }
}

static void project_cube(float x, float y, float z, float t, SDL_Point *p)
{
    float cx = cosf(t);
    float sx = sinf(t);
    float cy = cosf(t * 0.73f);
    float sy = sinf(t * 0.73f);
    float x1 = x * cx - z * sx;
    float z1 = x * sx + z * cx;
    float y1 = y * cy - z1 * sy;
    float z2 = y * sy + z1 * cy + 5.0f;
    float scale = 74.0f / z2;
    p->x = (int)(HYPNO_W * 0.5f + x1 * scale);
    p->y = (int)(HYPNO_H * 0.74f + y1 * scale);
}

static void draw_floor_and_cube(SDL_Renderer *renderer, float t)
{
    int i;
    static const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
    };
    static const float verts[8][3] = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
    };
    SDL_Point projected[8];

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (i = 0; i < 14; ++i) {
        float q = fmodf((float)i / 14.0f + t * 0.14f, 1.0f);
        int y = (int)(HYPNO_H * 0.62f + q * q * (HYPNO_H * 0.38f));
        int bright = 40 + (int)(q * 130.0f);
        SDL_SetRenderDrawColor(renderer, 45, (Uint8)bright, 225, 110);
        SDL_RenderDrawLine(renderer, 0, y, HYPNO_W - 1, y);
    }
    for (i = -10; i <= 10; ++i) {
        int x0 = HYPNO_W / 2 + i * 12;
        int x1 = HYPNO_W / 2 + i * 37;
        SDL_SetRenderDrawColor(renderer, 40, 150, 230, 95);
        SDL_RenderDrawLine(renderer, x0, (int)(HYPNO_H * 0.62f), x1, HYPNO_H - 1);
    }

    for (i = 0; i < 8; ++i) {
        project_cube(verts[i][0] * 1.25f,
                     verts[i][1] * 1.25f - 0.35f,
                     verts[i][2] * 1.25f,
                     t * 0.9f, &projected[i]);
    }
    SDL_SetRenderDrawColor(renderer, 255, 235, 95, 235);
    for (i = 0; i < 12; ++i) {
        SDL_RenderDrawLine(renderer,
                           projected[edges[i][0]].x, projected[edges[i][0]].y,
                           projected[edges[i][1]].x, projected[edges[i][1]].y);
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    Star stars[STARS];
    SDL_Event event;
    Uint32 start;
    int running = 1;
    int paused = 0;
    int direction = 1;
    float frozen_t = 0.0f;
    int i;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenHypno", HYPNO_W, HYPNO_H, 2)) {
        return 1;
    }
    for (i = 0; i < STARS; ++i) reset_star(&stars[i], 1);
    start = SDL_GetTicks();

    while (running) {
        float t = paused ? frozen_t : (SDL_GetTicks() - start) * 0.001f;
        char mode[96];
        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) continue;
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    paused = !paused;
                    if (paused) frozen_t = t;
                    else start = SDL_GetTicks() - (Uint32)(frozen_t * 1000.0f);
                } else if (event.key.keysym.sym == SDLK_LEFT ||
                           event.key.keysym.sym == SDLK_RIGHT) {
                    direction = -direction;
                }
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 2, 3, 14, 255);
        SDL_RenderClear(app.renderer);
        draw_starfield(app.renderer, stars, paused ? 0.0f : 0.009f,
                       HYPNO_W * 0.5f, HYPNO_H * 0.42f);
        draw_tunnel(app.renderer, t, direction);
        draw_floor_and_cube(app.renderer, t);
        SDL_RenderPresent(app.renderer);
        snprintf(mode, sizeof(mode), "%s direction %s - arrows/Space/F/Esc",
                 paused ? "paused" : "running", direction > 0 ? ">" : "<");
        demo_title_tick(&app, mode);
    }

    demo_shutdown(&app);
    return 0;
}
