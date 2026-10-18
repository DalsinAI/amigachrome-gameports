/*
 * OpenTriangles - SDL_RenderGeometry / OpenGPU TRIANGLES showcase.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>

#define TRI_W 320
#define TRI_H 240
#define TEX_SIZE 64

static SDL_Texture *make_checker(SDL_Renderer *renderer)
{
    Uint32 pixels[TEX_SIZE * TEX_SIZE];
    SDL_Texture *texture;
    int x;
    int y;

    for (y = 0; y < TEX_SIZE; ++y) {
        for (x = 0; x < TEX_SIZE; ++x) {
            int check = ((x >> 3) ^ (y >> 3)) & 1;
            int edge = (x < 2 || y < 2 || x >= TEX_SIZE - 2 || y >= TEX_SIZE - 2);
            if (edge) {
                pixels[y * TEX_SIZE + x] = demo_argb(255, 255, 255);
            } else if (check) {
                pixels[y * TEX_SIZE + x] = demo_argb(230, 35, 35);
            } else {
                pixels[y * TEX_SIZE + x] = demo_argb(238, 238, 238);
            }
        }
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, TEX_SIZE, TEX_SIZE);
    if (!texture) {
        return NULL;
    }
    SDL_UpdateTexture(texture, NULL, pixels, TEX_SIZE * (int)sizeof(Uint32));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

static void set_vertex(SDL_Vertex *v, float x, float y,
                       Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                       float u, float tex_v)
{
    v->position.x = x;
    v->position.y = y;
    v->color.r = r;
    v->color.g = g;
    v->color.b = b;
    v->color.a = a;
    v->tex_coord.x = u;
    v->tex_coord.y = tex_v;
}

static void build_triangle(SDL_Vertex v[3], float cx, float cy, float radius,
                           float angle, Uint8 alpha, int palette)
{
    static const Uint8 colours[3][3][3] = {
        {{255, 45, 45}, {45, 255, 85}, {55, 95, 255}},
        {{255, 210, 40}, {255, 65, 180}, {35, 225, 255}},
        {{245, 245, 245}, {230, 35, 35}, {245, 245, 245}}
    };
    static const float uv[3][2] = {{0.5f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    int i;

    for (i = 0; i < 3; ++i) {
        float a = angle + (float)i * 2.094395102f;
        set_vertex(&v[i],
                   cx + cosf(a) * radius,
                   cy + sinf(a) * radius,
                   colours[palette][i][0],
                   colours[palette][i][1],
                   colours[palette][i][2],
                   alpha,
                   uv[i][0], uv[i][1]);
    }
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *checker;
    int running = 1;
    int paused = 0;
    int textured = 1;
    int blend_index = 1;
    float angle = 0.0f;
    SDL_Event event;
    static const SDL_BlendMode blends[4] = {
        SDL_BLENDMODE_NONE,
        SDL_BLENDMODE_BLEND,
        SDL_BLENDMODE_ADD,
        SDL_BLENDMODE_MOD
    };
    static const char *blend_names[4] = {"none", "blend", "add", "mod"};
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenTriangles", TRI_W, TRI_H, 2)) {
        return 1;
    }
    checker = make_checker(app.renderer);
    if (!checker) {
        fprintf(stderr, "OpenTriangles: texture creation failed: %s\n", SDL_GetError());
        demo_shutdown(&app);
        return 2;
    }

    while (running) {
        SDL_Vertex big[3];
        SDL_Vertex left[3];
        SDL_Vertex right[3];
        char mode[112];

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        paused = !paused;
                        break;
                    case SDLK_t:
                        textured = !textured;
                        break;
                    case SDLK_b:
                        blend_index = (blend_index + 1) & 3;
                        break;
                    default:
                        break;
                }
            }
            if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
                angle += event.motion.xrel * 0.015f;
            }
        }

        if (!paused) {
            angle += 0.0125f;
        }

        SDL_SetTextureBlendMode(checker, blends[blend_index]);
        SDL_SetRenderDrawColor(app.renderer, 8, 10, 24, 255);
        SDL_RenderClear(app.renderer);

        build_triangle(big, TRI_W * 0.5f, TRI_H * 0.52f, 82.0f,
                       angle, blend_index == 0 ? 255 : 205, 2);
        build_triangle(left, 78.0f, 68.0f, 43.0f,
                       -angle * 1.55f, 220, 0);
        build_triangle(right, 244.0f, 67.0f, 40.0f,
                       angle * 2.1f, 210, 1);

        if (SDL_RenderGeometry(app.renderer, textured ? checker : NULL,
                               big, 3, NULL, 0) != 0) {
            fprintf(stderr, "OpenTriangles: SDL_RenderGeometry failed: %s\n", SDL_GetError());
            running = 0;
        }
        SDL_RenderGeometry(app.renderer, NULL, left, 3, NULL, 0);
        SDL_RenderGeometry(app.renderer, NULL, right, 3, NULL, 0);

        SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 90);
        SDL_RenderDrawLine(app.renderer, 12, TRI_H - 20, TRI_W - 12, TRI_H - 20);
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "%s, %s, %s - T/B/Space/F/Esc",
                 textured ? "textured" : "colour vertices",
                 blend_names[blend_index],
                 paused ? "paused" : "running");
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(checker);
    demo_shutdown(&app);
    return 0;
}
