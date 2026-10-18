/*
 * OpenBoids - flocking simulation rendered as one SDL_RenderGeometry batch.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdint.h>

#define VIEW_W 320
#define VIEW_H 240
#define MAX_BOIDS 128

typedef struct Boid {
    float x;
    float y;
    float vx;
    float vy;
} Boid;

static uint32_t rng_state = 0x0b01d5a5u;

static uint32_t next_random(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static float random_unit(void)
{
    return (float)(next_random() & 0xffffu) / 65535.0f;
}

static void reset_boid(Boid *b)
{
    float a = random_unit() * 6.28318530f;
    float speed = 0.65f + random_unit() * 1.25f;
    b->x = random_unit() * VIEW_W;
    b->y = random_unit() * VIEW_H;
    b->vx = cosf(a) * speed;
    b->vy = sinf(a) * speed;
}

static void limit_velocity(Boid *b)
{
    float speed2 = b->vx * b->vx + b->vy * b->vy;
    const float max_speed = 2.65f;
    const float min_speed = 0.38f;

    if (speed2 > max_speed * max_speed) {
        float scale = max_speed / sqrtf(speed2);
        b->vx *= scale;
        b->vy *= scale;
    } else if (speed2 < min_speed * min_speed) {
        float speed = sqrtf(speed2);
        if (speed < 0.0001f) {
            b->vx = min_speed;
            b->vy = 0.0f;
        } else {
            float scale = min_speed / speed;
            b->vx *= scale;
            b->vy *= scale;
        }
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    Boid boids[MAX_BOIDS];
    Boid next[MAX_BOIDS];
    SDL_Vertex vertices[MAX_BOIDS * 3];
    SDL_Event event;
    int running = 1;
    int paused = 0;
    int wrap_edges = 1;
    int colour_cycle = 1;
    int boid_count = 80;
    Uint32 frame = 0;
    int i;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenBoids", VIEW_W, VIEW_H, 2)) {
        return 1;
    }

    for (i = 0; i < MAX_BOIDS; ++i) {
        reset_boid(&boids[i]);
    }

    while (running) {
        Uint32 mouse_buttons;
        int mouse_x;
        int mouse_y;
        int window_w;
        int window_h;
        float logical_mouse_x;
        float logical_mouse_y;
        char mode[128];

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
                        boid_count += 8;
                        if (boid_count > MAX_BOIDS) boid_count = MAX_BOIDS;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        boid_count -= 8;
                        if (boid_count < 16) boid_count = 16;
                        break;
                    case SDLK_w:
                        wrap_edges = !wrap_edges;
                        break;
                    case SDLK_c:
                        colour_cycle = !colour_cycle;
                        break;
                    case SDLK_r:
                        for (i = 0; i < boid_count; ++i) reset_boid(&boids[i]);
                        break;
                    default:
                        break;
                }
            }
        }

        mouse_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
        SDL_GetWindowSize(app.window, &window_w, &window_h);
        logical_mouse_x = window_w > 0 ? (float)mouse_x * VIEW_W / window_w : 0.0f;
        logical_mouse_y = window_h > 0 ? (float)mouse_y * VIEW_H / window_h : 0.0f;

        if (!paused) {
            ++frame;
            for (i = 0; i < boid_count; ++i) {
                float center_x = 0.0f;
                float center_y = 0.0f;
                float align_x = 0.0f;
                float align_y = 0.0f;
                float separate_x = 0.0f;
                float separate_y = 0.0f;
                int neighbours = 0;
                int j;

                next[i] = boids[i];
                for (j = 0; j < boid_count; ++j) {
                    float dx;
                    float dy;
                    float d2;
                    if (i == j) continue;
                    dx = boids[j].x - boids[i].x;
                    dy = boids[j].y - boids[i].y;
                    d2 = dx * dx + dy * dy;
                    if (d2 < 34.0f * 34.0f) {
                        center_x += boids[j].x;
                        center_y += boids[j].y;
                        align_x += boids[j].vx;
                        align_y += boids[j].vy;
                        ++neighbours;
                        if (d2 < 12.0f * 12.0f && d2 > 0.01f) {
                            separate_x -= dx / d2;
                            separate_y -= dy / d2;
                        }
                    }
                }

                if (neighbours > 0) {
                    float inv = 1.0f / neighbours;
                    center_x = center_x * inv - boids[i].x;
                    center_y = center_y * inv - boids[i].y;
                    align_x = align_x * inv - boids[i].vx;
                    align_y = align_y * inv - boids[i].vy;
                    next[i].vx += center_x * 0.0015f + align_x * 0.035f + separate_x * 0.55f;
                    next[i].vy += center_y * 0.0015f + align_y * 0.035f + separate_y * 0.55f;
                }

                if (mouse_buttons & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {
                    float dx = logical_mouse_x - boids[i].x;
                    float dy = logical_mouse_y - boids[i].y;
                    float d2 = dx * dx + dy * dy + 20.0f;
                    float pull = (mouse_buttons & SDL_BUTTON_LMASK) ? 1.0f : -1.0f;
                    next[i].vx += dx * (55.0f / d2) * pull;
                    next[i].vy += dy * (55.0f / d2) * pull;
                }

                if (!wrap_edges) {
                    if (boids[i].x < 28.0f) next[i].vx += 0.06f;
                    if (boids[i].x > VIEW_W - 28.0f) next[i].vx -= 0.06f;
                    if (boids[i].y < 28.0f) next[i].vy += 0.06f;
                    if (boids[i].y > VIEW_H - 28.0f) next[i].vy -= 0.06f;
                }

                limit_velocity(&next[i]);
                next[i].x += next[i].vx;
                next[i].y += next[i].vy;

                if (wrap_edges) {
                    if (next[i].x < 0.0f) next[i].x += VIEW_W;
                    if (next[i].x >= VIEW_W) next[i].x -= VIEW_W;
                    if (next[i].y < 0.0f) next[i].y += VIEW_H;
                    if (next[i].y >= VIEW_H) next[i].y -= VIEW_H;
                }
            }
            for (i = 0; i < boid_count; ++i) {
                boids[i] = next[i];
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 3, 7, 15, 255);
        SDL_RenderClear(app.renderer);
        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app.renderer, 30, 90, 130, 52);
        for (i = 0; i < VIEW_W; i += 32) {
            SDL_RenderDrawLine(app.renderer, i, 0, i, VIEW_H - 1);
        }
        for (i = 0; i < VIEW_H; i += 24) {
            SDL_RenderDrawLine(app.renderer, 0, i, VIEW_W - 1, i);
        }

        for (i = 0; i < boid_count; ++i) {
            Boid *b = &boids[i];
            float speed = sqrtf(b->vx * b->vx + b->vy * b->vy);
            float dx = b->vx / speed;
            float dy = b->vy / speed;
            float sx = -dy;
            float sy = dx;
            Uint8 r = 90;
            Uint8 g = 210;
            Uint8 blue = 255;
            SDL_Vertex *v = &vertices[i * 3];

            if (colour_cycle) {
                float phase = i * 0.23f + frame * 0.018f;
                r = (Uint8)(128.0f + 127.0f * sinf(phase));
                g = (Uint8)(128.0f + 127.0f * sinf(phase + 2.094f));
                blue = (Uint8)(128.0f + 127.0f * sinf(phase + 4.188f));
            }

            v[0].tex_coord.x = 0.0f; v[0].tex_coord.y = 0.0f;
            v[1].tex_coord.x = 0.0f; v[1].tex_coord.y = 0.0f;
            v[2].tex_coord.x = 0.0f; v[2].tex_coord.y = 0.0f;
            v[0].position.x = b->x + dx * 7.0f;
            v[0].position.y = b->y + dy * 7.0f;
            v[1].position.x = b->x - dx * 4.5f + sx * 3.8f;
            v[1].position.y = b->y - dy * 4.5f + sy * 3.8f;
            v[2].position.x = b->x - dx * 4.5f - sx * 3.8f;
            v[2].position.y = b->y - dy * 4.5f - sy * 3.8f;
            v[0].color.r = r; v[0].color.g = g; v[0].color.b = blue; v[0].color.a = 255;
            v[1].color.r = r / 2; v[1].color.g = g / 2; v[1].color.b = blue / 2; v[1].color.a = 210;
            v[2].color = v[1].color;
        }

        SDL_RenderGeometry(app.renderer, NULL, vertices, boid_count * 3, NULL, 0);
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "%d boids %s%s - mouse L attract/R repel, arrows/W/C/R",
                 boid_count,
                 wrap_edges ? "wrap" : "walls",
                 paused ? " paused" : "");
        demo_title_tick(&app, mode);
    }

    demo_shutdown(&app);
    return 0;
}
