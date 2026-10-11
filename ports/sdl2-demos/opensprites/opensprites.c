/*
 * OpenSprites - alpha, colour modulation, rotation and copy stress.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define VIEW_W 320
#define VIEW_H 240
#define SPRITE_SIZE 24
#define MAX_SPRITES 256

typedef struct Sprite {
    float x;
    float y;
    float vx;
    float vy;
    float angle;
    float spin;
    float phase;
    float scale;
} Sprite;

static uint32_t rng_state = 0x53d1a7c9u;

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

static SDL_Texture *make_sprite_texture(SDL_Renderer *renderer)
{
    Uint32 pixels[SPRITE_SIZE * SPRITE_SIZE];
    SDL_Texture *texture;
    int x;
    int y;
    float center = (SPRITE_SIZE - 1) * 0.5f;
    float radius = SPRITE_SIZE * 0.48f;

    for (y = 0; y < SPRITE_SIZE; ++y) {
        for (x = 0; x < SPRITE_SIZE; ++x) {
            float dx = ((float)x - center) / radius;
            float dy = ((float)y - center) / radius;
            float d = sqrtf(dx * dx + dy * dy);
            Uint8 alpha = 0;
            Uint8 shade = 0;
            Uint32 pixel = 0;

            if (d < 1.0f) {
                float edge = 1.0f - d;
                int checker = (((x >> 2) ^ (y >> 2)) & 1);
                alpha = (Uint8)(edge > 0.20f ? 255.0f : edge * 1275.0f);
                shade = (Uint8)(checker ? 255 : 165);
                pixel = ((Uint32)alpha << 24) |
                        ((Uint32)shade << 16) |
                        ((Uint32)shade << 8) |
                        (Uint32)255;
            }
            pixels[y * SPRITE_SIZE + x] = pixel;
        }
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC,
                                SPRITE_SIZE, SPRITE_SIZE);
    if (!texture) {
        return NULL;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, NULL, pixels,
                      SPRITE_SIZE * (int)sizeof(Uint32));
    return texture;
}

static void reset_sprite(Sprite *sprite, int index)
{
    float speed = 0.45f + random_unit() * 1.65f;
    float angle = random_unit() * 6.28318530f;

    sprite->x = random_unit() * (VIEW_W - SPRITE_SIZE);
    sprite->y = random_unit() * (VIEW_H - SPRITE_SIZE);
    sprite->vx = cosf(angle) * speed;
    sprite->vy = sinf(angle) * speed;
    sprite->angle = random_unit() * 360.0f;
    sprite->spin = (random_unit() - 0.5f) * 4.0f;
    sprite->phase = (float)index * 0.37f;
    sprite->scale = 0.55f + random_unit() * 1.25f;
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *texture = NULL;
    Sprite sprites[MAX_SPRITES];
    SDL_Event event;
    int running = 1;
    int paused = 0;
    int colour_cycle = 1;
    int sprite_count = 128;
    int blend_index = 0;
    Uint32 frame = 0;
    int i;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenSprites", VIEW_W, VIEW_H, 2)) {
        return 1;
    }

    texture = make_sprite_texture(app.renderer);
    if (!texture) {
        fprintf(stderr, "OpenSprites: texture creation failed: %s\n", SDL_GetError());
        demo_shutdown(&app);
        return 2;
    }

    for (i = 0; i < MAX_SPRITES; ++i) {
        reset_sprite(&sprites[i], i);
    }

    while (running) {
        const char *blend_name = "alpha";
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
                        sprite_count += 16;
                        if (sprite_count > MAX_SPRITES) sprite_count = MAX_SPRITES;
                        break;
                    case SDLK_DOWN:
                    case SDLK_LEFT:
                        sprite_count -= 16;
                        if (sprite_count < 16) sprite_count = 16;
                        break;
                    case SDLK_b:
                        blend_index = (blend_index + 1) % 3;
                        break;
                    case SDLK_c:
                        colour_cycle = !colour_cycle;
                        break;
                    default:
                        break;
                }
            }
        }

        if (blend_index == 0) {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            blend_name = "alpha";
        } else if (blend_index == 1) {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
            blend_name = "add";
        } else {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
            blend_name = "mod";
        }

        if (!paused) {
            ++frame;
            for (i = 0; i < sprite_count; ++i) {
                Sprite *s = &sprites[i];
                float size = SPRITE_SIZE * s->scale;
                s->x += s->vx;
                s->y += s->vy;
                s->angle += s->spin;

                if (s->x < 0.0f) {
                    s->x = 0.0f;
                    s->vx = fabsf(s->vx);
                } else if (s->x + size >= VIEW_W) {
                    s->x = VIEW_W - size - 1.0f;
                    s->vx = -fabsf(s->vx);
                }
                if (s->y < 0.0f) {
                    s->y = 0.0f;
                    s->vy = fabsf(s->vy);
                } else if (s->y + size >= VIEW_H) {
                    s->y = VIEW_H - size - 1.0f;
                    s->vy = -fabsf(s->vy);
                }
            }
        }

        SDL_SetRenderDrawColor(app.renderer, 5, 8, 18, 255);
        SDL_RenderClear(app.renderer);
        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        for (i = 0; i < VIEW_H; i += 20) {
            Uint8 alpha = (Uint8)(18 + i / 5);
            SDL_SetRenderDrawColor(app.renderer, 32, 80, 130, alpha);
            SDL_RenderDrawLine(app.renderer, 0, i, VIEW_W - 1, i);
        }

        for (i = 0; i < sprite_count; ++i) {
            Sprite *s = &sprites[i];
            SDL_Rect dst;
            float wave = sinf((float)frame * 0.025f + s->phase);
            Uint8 r = 255;
            Uint8 g = 255;
            Uint8 b = 255;
            Uint8 a = (Uint8)(145.0f + 105.0f * (0.5f + 0.5f * wave));

            if (colour_cycle) {
                r = (Uint8)(128.0f + 127.0f * sinf(s->phase + frame * 0.013f));
                g = (Uint8)(128.0f + 127.0f * sinf(s->phase + frame * 0.017f + 2.094f));
                b = (Uint8)(128.0f + 127.0f * sinf(s->phase + frame * 0.019f + 4.188f));
            }

            dst.w = (int)(SPRITE_SIZE * s->scale);
            dst.h = dst.w;
            dst.x = (int)s->x;
            dst.y = (int)s->y;
            SDL_SetTextureColorMod(texture, r, g, b);
            SDL_SetTextureAlphaMod(texture, a);
            SDL_RenderCopyEx(app.renderer, texture, NULL, &dst,
                             s->angle, NULL, SDL_FLIP_NONE);
        }

        SDL_RenderPresent(app.renderer);
        snprintf(mode, sizeof(mode), "%d sprites %s%s - arrows/B/C/Space",
                 sprite_count,
                 blend_name,
                 paused ? " paused" : "");
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(texture);
    demo_shutdown(&app);
    return 0;
}
