/*
 * OpenParticles - additive and alpha particle fountain for OpenGPU SDL2.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define VIEW_W 320
#define VIEW_H 240
#define PARTICLE_SIZE 12
#define MAX_PARTICLES 768

typedef struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float life;
    float max_life;
    float spin;
    float angle;
    Uint8 hue;
} Particle;

static uint32_t rng_state = 0x90a5c317u;

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

static SDL_Texture *make_particle_texture(SDL_Renderer *renderer)
{
    Uint32 pixels[PARTICLE_SIZE * PARTICLE_SIZE];
    SDL_Texture *texture;
    int x;
    int y;
    float center = (PARTICLE_SIZE - 1) * 0.5f;
    float radius = PARTICLE_SIZE * 0.5f;

    for (y = 0; y < PARTICLE_SIZE; ++y) {
        for (x = 0; x < PARTICLE_SIZE; ++x) {
            float dx = ((float)x - center) / radius;
            float dy = ((float)y - center) / radius;
            float d = sqrtf(dx * dx + dy * dy);
            Uint8 a = 0;
            Uint8 c = 0;
            if (d < 1.0f) {
                float glow = 1.0f - d;
                a = (Uint8)(glow * glow * 255.0f);
                c = (Uint8)(120.0f + glow * 135.0f);
            }
            pixels[y * PARTICLE_SIZE + x] = ((Uint32)a << 24) |
                                                  ((Uint32)c << 16) |
                                                  ((Uint32)c << 8) |
                                                  (Uint32)c;
        }
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC,
                                PARTICLE_SIZE, PARTICLE_SIZE);
    if (!texture) {
        return NULL;
    }
    SDL_UpdateTexture(texture, NULL, pixels,
                      PARTICLE_SIZE * (int)sizeof(Uint32));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
    return texture;
}

static void respawn_particle(Particle *p, int index)
{
    float spread = (random_unit() - 0.5f) * 2.0f;
    float speed = 1.4f + random_unit() * 2.6f;

    p->x = VIEW_W * 0.5f + (random_unit() - 0.5f) * 20.0f;
    p->y = VIEW_H - 26.0f + random_unit() * 5.0f;
    p->vx = spread * speed;
    p->vy = -(2.2f + random_unit() * 3.8f);
    p->life = 85.0f + random_unit() * 105.0f;
    p->max_life = p->life;
    p->spin = (random_unit() - 0.5f) * 8.0f;
    p->angle = random_unit() * 360.0f;
    p->hue = (Uint8)((index * 11 + (int)(random_unit() * 64.0f)) & 255);
}

static void hue_to_rgb(Uint8 hue, Uint8 *r, Uint8 *g, Uint8 *b)
{
    unsigned sector = (unsigned)hue / 43u;
    unsigned offset = ((unsigned)hue - sector * 43u) * 6u;
    Uint8 q = (Uint8)(255u - offset);
    Uint8 t = (Uint8)offset;

    switch (sector) {
        case 0: *r = 255; *g = t;   *b = 40;  break;
        case 1: *r = q;   *g = 255; *b = 40;  break;
        case 2: *r = 40;  *g = 255; *b = t;   break;
        case 3: *r = 40;  *g = q;   *b = 255; break;
        case 4: *r = t;   *g = 40;  *b = 255; break;
        default:*r = 255; *g = 40;  *b = q;   break;
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *particle_texture = NULL;
    Particle particles[MAX_PARTICLES];
    SDL_Event event;
    int running = 1;
    int paused = 0;
    int additive = 1;
    int gravity = 1;
    int particle_count = 384;
    Uint32 frame = 0;
    int i;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenParticles", VIEW_W, VIEW_H, 2)) {
        return 1;
    }

    particle_texture = make_particle_texture(app.renderer);
    if (!particle_texture) {
        fprintf(stderr, "OpenParticles: texture creation failed: %s\n", SDL_GetError());
        demo_shutdown(&app);
        return 2;
    }

    for (i = 0; i < MAX_PARTICLES; ++i) {
        respawn_particle(&particles[i], i);
        particles[i].life *= random_unit();
    }

    while (running) {
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
                        particle_count += 64;
                        if (particle_count > MAX_PARTICLES) particle_count = MAX_PARTICLES;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        particle_count -= 64;
                        if (particle_count < 64) particle_count = 64;
                        break;
                    case SDLK_b:
                        additive = !additive;
                        break;
                    case SDLK_g:
                        gravity = !gravity;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) {
            ++frame;
            for (i = 0; i < particle_count; ++i) {
                Particle *p = &particles[i];
                p->life -= 1.0f;
                if (p->life <= 0.0f || p->x < -20.0f || p->x > VIEW_W + 20.0f || p->y < -30.0f) {
                    respawn_particle(p, i + (int)frame);
                    continue;
                }
                p->x += p->vx;
                p->y += p->vy;
                p->vx += sinf((float)frame * 0.025f + i * 0.17f) * 0.008f;
                if (gravity) {
                    p->vy += 0.035f;
                }
                p->angle += p->spin;
            }
        }

        SDL_SetTextureBlendMode(particle_texture,
                                additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app.renderer, 2, 3, 12, 255);
        SDL_RenderClear(app.renderer);

        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        for (i = 0; i < 9; ++i) {
            SDL_Rect band = { 0, VIEW_H - 18 - i * 8, VIEW_W, 2 };
            SDL_SetRenderDrawColor(app.renderer,
                                   (Uint8)(20 + i * 7),
                                   (Uint8)(30 + i * 9),
                                   (Uint8)(70 + i * 12),
                                   (Uint8)(18 + i * 4));
            SDL_RenderFillRect(app.renderer, &band);
        }

        for (i = 0; i < particle_count; ++i) {
            Particle *p = &particles[i];
            float life_ratio = p->life / p->max_life;
            float scale = 0.45f + life_ratio * 1.2f;
            SDL_Rect dst;
            Uint8 r;
            Uint8 g;
            Uint8 b;
            Uint8 alpha;

            if (life_ratio < 0.0f) life_ratio = 0.0f;
            if (life_ratio > 1.0f) life_ratio = 1.0f;
            hue_to_rgb((Uint8)(p->hue + frame / 3u), &r, &g, &b);
            alpha = (Uint8)(life_ratio * 255.0f);
            dst.w = (int)(PARTICLE_SIZE * scale);
            dst.h = dst.w;
            dst.x = (int)(p->x - dst.w * 0.5f);
            dst.y = (int)(p->y - dst.h * 0.5f);
            SDL_SetTextureColorMod(particle_texture, r, g, b);
            SDL_SetTextureAlphaMod(particle_texture, alpha);
            SDL_RenderCopyEx(app.renderer, particle_texture, NULL, &dst,
                             p->angle, NULL, SDL_FLIP_NONE);
        }

        SDL_RenderPresent(app.renderer);
        snprintf(mode, sizeof(mode), "%d particles %s gravity=%s%s - arrows/B/G/Space",
                 particle_count,
                 additive ? "additive" : "alpha",
                 gravity ? "on" : "off",
                 paused ? " paused" : "");
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(particle_texture);
    demo_shutdown(&app);
    return 0;
}
