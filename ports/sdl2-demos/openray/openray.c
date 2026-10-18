/*
 * OpenRay - compact interactive SDL2 raycaster for OpenUp.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#include "../common/demo_common.h"
#include <math.h>
#include <stdlib.h>

#define RAY_W 320
#define RAY_H 200
#define MAP_W 16
#define MAP_H 16

static const char world[MAP_H][MAP_W + 1] = {
    "1111111111111111",
    "1000000000000001",
    "1022220000333301",
    "1020020000300301",
    "1020020000300301",
    "1022220000333301",
    "1000000000000001",
    "1000111100000001",
    "1000100100444401",
    "1000100100400401",
    "1000111100400401",
    "1000000000444401",
    "1055550000000001",
    "1050050000666601",
    "1055550000600601",
    "1111111111111111"
};

static Uint32 wall_colour(int cell, int side, int distance_band)
{
    static const Uint8 base[7][3] = {
        {0, 0, 0},
        {220, 50, 45},
        {55, 190, 90},
        {60, 115, 235},
        {225, 195, 45},
        {190, 65, 220},
        {35, 205, 220}
    };
    int shade = 255 - distance_band * 14 - side * 46;
    int r;
    int g;
    int b;
    if (shade < 54) shade = 54;
    if (cell < 1 || cell > 6) cell = 1;
    r = base[cell][0] * shade / 255;
    g = base[cell][1] * shade / 255;
    b = base[cell][2] * shade / 255;
    return demo_argb((Uint8)r, (Uint8)g, (Uint8)b);
}

static int map_open(double x, double y)
{
    int mx = (int)x;
    int my = (int)y;
    if (mx < 0 || my < 0 || mx >= MAP_W || my >= MAP_H) return 0;
    return world[my][mx] == '0';
}

static void rotate_view(double *dir_x, double *dir_y,
                        double *plane_x, double *plane_y, double amount)
{
    double old_dir_x = *dir_x;
    double old_plane_x = *plane_x;
    double cs = cos(amount);
    double sn = sin(amount);
    *dir_x = *dir_x * cs - *dir_y * sn;
    *dir_y = old_dir_x * sn + *dir_y * cs;
    *plane_x = *plane_x * cs - *plane_y * sn;
    *plane_y = old_plane_x * sn + *plane_y * cs;
}

static void render_scene(Uint32 *pixels, double pos_x, double pos_y,
                         double dir_x, double dir_y,
                         double plane_x, double plane_y)
{
    int x;
    int y;

    for (y = 0; y < RAY_H; ++y) {
        Uint32 colour;
        if (y < RAY_H / 2) {
            int shade = 22 + y * 32 / (RAY_H / 2);
            colour = demo_argb((Uint8)(shade / 2), (Uint8)(shade / 2), (Uint8)shade);
        } else {
            int shade = 72 - (y - RAY_H / 2) * 46 / (RAY_H / 2);
            colour = demo_argb((Uint8)shade, (Uint8)(shade * 3 / 4), (Uint8)(shade / 2));
        }
        for (x = 0; x < RAY_W; ++x) {
            pixels[y * RAY_W + x] = colour;
        }
    }

    for (x = 0; x < RAY_W; ++x) {
        double camera_x = 2.0 * x / (double)RAY_W - 1.0;
        double ray_x = dir_x + plane_x * camera_x;
        double ray_y = dir_y + plane_y * camera_x;
        int map_x = (int)pos_x;
        int map_y = (int)pos_y;
        double delta_x = ray_x == 0.0 ? 1.0e30 : fabs(1.0 / ray_x);
        double delta_y = ray_y == 0.0 ? 1.0e30 : fabs(1.0 / ray_y);
        double side_x;
        double side_y;
        double distance;
        int step_x;
        int step_y;
        int side = 0;
        int hit = 0;
        int cell = 1;
        int guard = 0;

        if (ray_x < 0.0) {
            step_x = -1;
            side_x = (pos_x - map_x) * delta_x;
        } else {
            step_x = 1;
            side_x = (map_x + 1.0 - pos_x) * delta_x;
        }
        if (ray_y < 0.0) {
            step_y = -1;
            side_y = (pos_y - map_y) * delta_y;
        } else {
            step_y = 1;
            side_y = (map_y + 1.0 - pos_y) * delta_y;
        }

        while (!hit && guard++ < 64) {
            if (side_x < side_y) {
                side_x += delta_x;
                map_x += step_x;
                side = 0;
            } else {
                side_y += delta_y;
                map_y += step_y;
                side = 1;
            }
            if (map_x < 0 || map_y < 0 || map_x >= MAP_W || map_y >= MAP_H) {
                hit = 1;
                cell = 1;
            } else if (world[map_y][map_x] != '0') {
                hit = 1;
                cell = world[map_y][map_x] - '0';
            }
        }

        if (side == 0) {
            distance = (map_x - pos_x + (1 - step_x) * 0.5) / ray_x;
        } else {
            distance = (map_y - pos_y + (1 - step_y) * 0.5) / ray_y;
        }
        if (distance < 0.05) distance = 0.05;
        {
            int line_h = (int)(RAY_H / distance);
            int start = -line_h / 2 + RAY_H / 2;
            int end = line_h / 2 + RAY_H / 2;
            Uint32 colour = wall_colour(cell, side, (int)distance);
            if (start < 0) start = 0;
            if (end >= RAY_H) end = RAY_H - 1;
            for (y = start; y <= end; ++y) {
                pixels[y * RAY_W + x] = colour;
            }
        }
    }
}

static void draw_minimap(SDL_Renderer *renderer, double pos_x, double pos_y,
                         double dir_x, double dir_y)
{
    int x;
    int y;
    const int scale = 3;
    SDL_Rect r;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    r.x = 4; r.y = 4; r.w = MAP_W * scale + 4; r.h = MAP_H * scale + 4;
    SDL_RenderFillRect(renderer, &r);
    for (y = 0; y < MAP_H; ++y) {
        for (x = 0; x < MAP_W; ++x) {
            if (world[y][x] != '0') {
                int cell = world[y][x] - '0';
                Uint32 c = wall_colour(cell, 0, 0);
                SDL_SetRenderDrawColor(renderer,
                    (Uint8)((c >> 16) & 255u),
                    (Uint8)((c >> 8) & 255u),
                    (Uint8)(c & 255u), 235);
                r.x = 6 + x * scale; r.y = 6 + y * scale;
                r.w = scale; r.h = scale;
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    r.x = 6 + (int)(pos_x * scale) - 1;
    r.y = 6 + (int)(pos_y * scale) - 1;
    r.w = 3; r.h = 3;
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderDrawLine(renderer,
        6 + (int)(pos_x * scale), 6 + (int)(pos_y * scale),
        6 + (int)((pos_x + dir_x * 2.2) * scale),
        6 + (int)((pos_y + dir_y * 2.2) * scale));
}

int main(int argc, char **argv)
{
    DemoApp app;
    SDL_Texture *texture;
    Uint32 *pixels;
    SDL_Event event;
    double pos_x = 8.0;
    double pos_y = 7.0;
    double dir_x = -1.0;
    double dir_y = 0.0;
    double plane_x = 0.0;
    double plane_y = 0.66;
    Uint32 previous = 0;
    int running = 1;
    int minimap = 1;
    (void)argc;
    (void)argv;

    if (!demo_init(&app, "OpenRay", RAY_W, RAY_H, 2)) {
        return 1;
    }
    pixels = (Uint32 *)malloc((size_t)RAY_W * RAY_H * sizeof(*pixels));
    texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, RAY_W, RAY_H);
    if (!pixels || !texture) {
        fprintf(stderr, "OpenRay: allocation failed: %s\n", SDL_GetError());
        free(pixels);
        if (texture) SDL_DestroyTexture(texture);
        demo_shutdown(&app);
        return 2;
    }
    previous = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        double dt = (double)(now - previous) / 1000.0;
        const Uint8 *keys;
        double move;
        double turn;
        char mode[112];
        previous = now;
        if (dt > 0.08) dt = 0.08;

        while (SDL_PollEvent(&event)) {
            if (demo_common_event(&app, &event, &running)) {
                continue;
            }
            if (event.type == SDL_KEYDOWN && !event.key.repeat &&
                event.key.keysym.sym == SDLK_m) {
                minimap = !minimap;
            }
            if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
                rotate_view(&dir_x, &dir_y, &plane_x, &plane_y,
                            event.motion.xrel * -0.0045);
            }
        }

        keys = SDL_GetKeyboardState(NULL);
        move = dt * (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ? 5.0 : 2.7);
        turn = dt * 1.9;
        if (keys[SDL_SCANCODE_LEFT]) rotate_view(&dir_x, &dir_y, &plane_x, &plane_y, turn);
        if (keys[SDL_SCANCODE_RIGHT]) rotate_view(&dir_x, &dir_y, &plane_x, &plane_y, -turn);

        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
            double nx = pos_x + dir_x * move;
            double ny = pos_y + dir_y * move;
            if (map_open(nx, pos_y)) pos_x = nx;
            if (map_open(pos_x, ny)) pos_y = ny;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
            double nx = pos_x - dir_x * move;
            double ny = pos_y - dir_y * move;
            if (map_open(nx, pos_y)) pos_x = nx;
            if (map_open(pos_x, ny)) pos_y = ny;
        }
        if (keys[SDL_SCANCODE_A]) {
            double nx = pos_x - dir_y * move;
            double ny = pos_y + dir_x * move;
            if (map_open(nx, pos_y)) pos_x = nx;
            if (map_open(pos_x, ny)) pos_y = ny;
        }
        if (keys[SDL_SCANCODE_D]) {
            double nx = pos_x + dir_y * move;
            double ny = pos_y - dir_x * move;
            if (map_open(nx, pos_y)) pos_x = nx;
            if (map_open(pos_x, ny)) pos_y = ny;
        }

        render_scene(pixels, pos_x, pos_y, dir_x, dir_y, plane_x, plane_y);
        SDL_UpdateTexture(texture, NULL, pixels, RAY_W * (int)sizeof(Uint32));
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, texture, NULL, NULL);
        if (minimap) draw_minimap(app.renderer, pos_x, pos_y, dir_x, dir_y);
        SDL_RenderPresent(app.renderer);

        snprintf(mode, sizeof(mode), "WASD/arrows, Shift run, drag turn, M map, F/Esc%s",
                 minimap ? "" : " - map hidden");
        demo_title_tick(&app, mode);
    }

    SDL_DestroyTexture(texture);
    free(pixels);
    demo_shutdown(&app);
    return 0;
}
