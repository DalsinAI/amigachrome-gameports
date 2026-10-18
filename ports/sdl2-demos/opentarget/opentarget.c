/*
 * OpenTarget - render-to-texture, alpha composition and rotation demo.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>

#define VIEW_W 320
#define VIEW_H 240
#define TARGET_W 160
#define TARGET_H 120

static void draw_target_scene(SDL_Renderer *renderer, float angle, int trails)
{
    int i;
    SDL_Vertex tri[3];
    float cx = TARGET_W * 0.5f;
    float cy = TARGET_H * 0.5f;
    float r = 38.0f;

    if (!trails) {
        SDL_SetRenderDrawColor(renderer, 4, 6, 18, 255);
        SDL_RenderClear(renderer);
    } else {
        SDL_Rect fade = { 0, 0, TARGET_W, TARGET_H };
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 2, 4, 14, 42);
        SDL_RenderFillRect(renderer, &fade);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (i = 0; i < 12; ++i) {
        float phase = angle * 0.0174532925f + (float)i * 0.5235987756f;
        SDL_Rect box;
        Uint8 alpha = (Uint8)(70 + i * 12);
        box.w = 10 + (i & 3) * 4;
        box.h = box.w;
        box.x = (int)(cx + cosf(phase) * (24.0f + i * 2.0f) - box.w * 0.5f);
        box.y = (int)(cy + sinf(phase * 1.3f) * (18.0f + i * 1.6f) - box.h * 0.5f);
        SDL_SetRenderDrawColor(renderer,
                               (Uint8)(30 + i * 17),
                               (Uint8)(220 - i * 11),
                               (Uint8)(90 + i * 13),
                               alpha);
        SDL_RenderFillRect(renderer, &box);
    }

    SDL_zeroa(tri);
    for (i = 0; i < 3; ++i) {
        float a = angle * 0.0174532925f + (float)i * 2.0943951024f;
        tri[i].position.x = cx + cosf(a) * r;
        tri[i].position.y = cy + sinf(a) * r;
        tri[i].color.r = (i == 0) ? 255 : 30;
        tri[i].color.g = (i == 1) ? 255 : 30;
        tri[i].color.b = (i == 2) ? 255 : 30;
        tri[i].color.a = 230;
    }
    SDL_RenderGeometry(renderer, NULL, tri, 3, NULL, 0);

    SDL_SetRenderDrawColor(renderer, 230, 235, 255, 220);
    SDL_RenderDrawLine(renderer, (int)cx - 55, (int)cy, (int)cx + 55, (int)cy);
    SDL_RenderDrawLine(renderer, (int)cx, (int)cy - 45, (int)cx, (int)cy + 45);
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *target = NULL;
    SDL_Event event;
    int running = 1;
    int paused = 0;
    int trails = 1;
    int blend_index = 0;
    float angle = 0.0f;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenTarget", VIEW_W, VIEW_H, 2)) {
        return 1;
    }

    target = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_TARGET,
                               TARGET_W, TARGET_H);
    if (!target) {
        fprintf(stderr, "OpenTarget: target texture unavailable: %s\n", SDL_GetError());
        demo_shutdown(&app);
        return 2;
    }
    SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);

    if (SDL_SetRenderTarget(app.renderer, target) != 0) {
        fprintf(stderr, "OpenTarget: SDL_SetRenderTarget failed: %s\n", SDL_GetError());
        SDL_DestroyTexture(target);
        demo_shutdown(&app);
        return 3;
    }
    SDL_SetRenderDrawColor(app.renderer, 4, 6, 18, 255);
    SDL_RenderClear(app.renderer);
    SDL_SetRenderTarget(app.renderer, NULL);

    while (running) {
        SDL_Rect main_dst = { 40, 30, 240, 180 };
        SDL_Rect orbit_a = { 16, 12, 96, 72 };
        SDL_Rect orbit_b = { 208, 156, 96, 72 };
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
                    case SDLK_b:
                        blend_index = (blend_index + 1) % 3;
                        break;
                    case SDLK_t:
                        trails = !trails;
                        break;
                    case SDLK_r:
                        angle = 0.0f;
                        break;
                    default:
                        break;
                }
            }
        }

        if (!paused) {
            angle += 0.85f;
            if (angle >= 360.0f) angle -= 360.0f;
        }

        SDL_SetRenderTarget(app.renderer, target);
        draw_target_scene(app.renderer, angle, trails);
        SDL_SetRenderTarget(app.renderer, NULL);

        if (blend_index == 0) {
            SDL_SetTextureBlendMode(target, SDL_BLENDMODE_BLEND);
            blend_name = "alpha";
        } else if (blend_index == 1) {
            SDL_SetTextureBlendMode(target, SDL_BLENDMODE_ADD);
            blend_name = "add";
        } else {
            SDL_SetTextureBlendMode(target, SDL_BLENDMODE_MOD);
            blend_name = "mod";
        }

        SDL_SetRenderDrawColor(app.renderer, 8, 10, 22, 255);
        SDL_RenderClear(app.renderer);

        SDL_SetTextureColorMod(target, 255, 255, 255);
        SDL_SetTextureAlphaMod(target, 220);
        SDL_RenderCopyEx(app.renderer, target, NULL, &main_dst,
                         angle * 0.16, NULL, SDL_FLIP_NONE);

        SDL_SetTextureColorMod(target, 255, 90, 170);
        SDL_SetTextureAlphaMod(target, 165);
        SDL_RenderCopyEx(app.renderer, target, NULL, &orbit_a,
                         -angle * 1.7, NULL, SDL_FLIP_HORIZONTAL);

        SDL_SetTextureColorMod(target, 80, 210, 255);
        SDL_SetTextureAlphaMod(target, 165);
        SDL_RenderCopyEx(app.renderer, target, NULL, &orbit_b,
                         angle * 1.3, NULL, SDL_FLIP_VERTICAL);

        SDL_RenderPresent(app.renderer);
        snprintf(mode, sizeof(mode), "target texture %s trails=%s%s - B/T/R/Space",
                 blend_name,
                 trails ? "on" : "off",
                 paused ? " paused" : "");
        demo_title_tick(&app, mode);
    }

    SDL_SetRenderTarget(app.renderer, NULL);
    SDL_DestroyTexture(target);
    demo_shutdown(&app);
    return 0;
}
