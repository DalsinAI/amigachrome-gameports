/*
 * OpenUp SDL2 demos - shared application shell.
 * Copyright (c) 2026 Dalsin Limited. MIT licence.
 */
#ifndef OPENUP_DEMO_COMMON_H
#define OPENUP_DEMO_COMMON_H

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

typedef struct DemoApp {
    SDL_Window *window;
    SDL_Renderer *renderer;
    const char *name;
    int logical_w;
    int logical_h;
    Uint32 title_epoch;
    Uint32 title_frames;
    Uint32 renderer_flags;
    char renderer_label[96];
} DemoApp;

static Uint32 demo_argb(Uint8 r, Uint8 g, Uint8 b)
{
    return 0xff000000u | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

static void demo_capture_renderer(DemoApp *app)
{
    SDL_RendererInfo info;
    const char *name = "unknown";
    const char *kind = "UNCLASSIFIED";
    char title[192];

    SDL_zero(info);
    if (SDL_GetRendererInfo(app->renderer, &info) == 0) {
        if (info.name && *info.name) {
            name = info.name;
        }
        app->renderer_flags = info.flags;
        if (info.flags & SDL_RENDERER_ACCELERATED) {
            kind = "ACCELERATED";
        } else if (info.flags & SDL_RENDERER_SOFTWARE) {
            kind = "SOFTWARE/CPU";
        }

        snprintf(app->renderer_label, sizeof(app->renderer_label),
                 "%s [%s%s%s]",
                 name,
                 kind,
                 (info.flags & SDL_RENDERER_PRESENTVSYNC) ? "+VSYNC" : "",
                 (info.flags & SDL_RENDERER_TARGETTEXTURE) ? "+TARGET" : "");

        fprintf(stdout,
                "%s: SDL renderer=%s kind=%s flags=0x%08lx texture-formats=%u\n",
                app->name,
                name,
                kind,
                (unsigned long)info.flags,
                (unsigned)info.num_texture_formats);
        fflush(stdout);
    } else {
        snprintf(app->renderer_label, sizeof(app->renderer_label),
                 "unknown [SDL_GetRendererInfo failed]");
        fprintf(stderr, "%s: SDL_GetRendererInfo failed: %s\n",
                app->name, SDL_GetError());
    }

    snprintf(title, sizeof(title), "%s | %s",
             app->name, app->renderer_label);
    SDL_SetWindowTitle(app->window, title);
}

static int demo_init(DemoApp *app, const char *name, int logical_w, int logical_h, int scale)
{
    Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    memset(app, 0, sizeof(*app));
    app->name = name;
    app->logical_w = logical_w;
    app->logical_h = logical_h;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "%s: SDL_Init failed: %s\n", name, SDL_GetError());
        return 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    app->window = SDL_CreateWindow(name,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   logical_w * scale,
                                   logical_h * scale,
                                   window_flags);
    if (!app->window) {
        fprintf(stderr, "%s: window creation failed: %s\n", name, SDL_GetError());
        SDL_Quit();
        return 0;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1,
                                       SDL_RENDERER_ACCELERATED |
                                       SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr,
                "%s: accelerated renderer unavailable (%s); trying SDL software fallback\n",
                name, SDL_GetError());
        app->renderer = SDL_CreateRenderer(app->window, -1, 0);
    }
    if (!app->renderer) {
        fprintf(stderr, "%s: renderer creation failed: %s\n", name, SDL_GetError());
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        memset(app, 0, sizeof(*app));
        return 0;
    }

    if (SDL_RenderSetLogicalSize(app->renderer, logical_w, logical_h) != 0) {
        fprintf(stderr, "%s: logical size failed: %s\n", name, SDL_GetError());
    }

    demo_capture_renderer(app);
    app->title_epoch = SDL_GetTicks();
    app->title_frames = 0;
    return 1;
}

static void demo_toggle_fullscreen(DemoApp *app)
{
    Uint32 flags = SDL_GetWindowFlags(app->window);
    Uint32 next = (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0u : SDL_WINDOW_FULLSCREEN_DESKTOP;
    SDL_SetWindowFullscreen(app->window, next);
}

/* Returns 1 when the event was a common command and should not be handled again. */
static int demo_common_event(DemoApp *app, const SDL_Event *event, int *running)
{
    if (event->type == SDL_QUIT) {
        *running = 0;
        return 1;
    }
    if (event->type == SDL_KEYDOWN && !event->key.repeat) {
        SDL_Keycode key = event->key.keysym.sym;
        if (key == SDLK_ESCAPE) {
            *running = 0;
            return 1;
        }
        if (key == SDLK_f || key == SDLK_F11) {
            demo_toggle_fullscreen(app);
            return 1;
        }
    }
    return 0;
}

static void demo_title_tick(DemoApp *app, const char *mode)
{
    Uint32 now;
    Uint32 elapsed;
    char title[256];

    ++app->title_frames;
    now = SDL_GetTicks();
    elapsed = now - app->title_epoch;
    if (elapsed < 1000u) {
        return;
    }

    if (mode && *mode) {
        snprintf(title, sizeof(title), "%s | %s | %s | %.1f fps",
                 app->name,
                 app->renderer_label,
                 mode,
                 (double)app->title_frames * 1000.0 / (double)elapsed);
    } else {
        snprintf(title, sizeof(title), "%s | %s | %.1f fps",
                 app->name,
                 app->renderer_label,
                 (double)app->title_frames * 1000.0 / (double)elapsed);
    }
    SDL_SetWindowTitle(app->window, title);
    app->title_epoch = now;
    app->title_frames = 0;
}

static void demo_shutdown(DemoApp *app)
{
    if (app->renderer) {
        SDL_DestroyRenderer(app->renderer);
    }
    if (app->window) {
        SDL_DestroyWindow(app->window);
    }
    SDL_Quit();
    memset(app, 0, sizeof(*app));
}

#endif
