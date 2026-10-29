/* SPDX-License-Identifier: MIT
 * Queue real SDL key events into FS-UAE for a repeatable guest UI run.
 * Build against SDL2's genuine headers; this does not call guest game APIs.
 * FS_INPUT_SEQUENCE: comma-separated frame:scancode:keycode triples.
 */
#define _GNU_SOURCE
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef void (*Swap)(SDL_Window *);
static struct Key { unsigned frame; int scan, sym; } keys[64];
void SDL_GL_SwapWindow(SDL_Window *window) {
    static Swap real_swap;
    static int (*push)(SDL_Event *);
    static Uint32 (*window_id)(SDL_Window *);
    static unsigned frame, count;
    static int configured;
    if (!real_swap) real_swap = (Swap)dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
    if (!push) push = (int (*)(SDL_Event *))dlsym(RTLD_NEXT, "SDL_PushEvent");
    if (!window_id) window_id = (Uint32 (*)(SDL_Window *))dlsym(RTLD_NEXT, "SDL_GetWindowID");
    if (!configured) {
        const char *spec = getenv("FS_INPUT_SEQUENCE");
        char *copy = spec ? strdup(spec) : NULL;
        if (copy) {
            for (char *item = strtok(copy, ","); item && count < 64; item = strtok(NULL, ",")) {
                struct Key k;
                if (sscanf(item, "%u:%d:%d", &k.frame, &k.scan, &k.sym) == 3) keys[count++] = k;
            }
            free(copy);
        }
        configured = 1;
    }
    ++frame;
    for (unsigned i = 0; i < count && push && window_id; ++i) {
        if (frame == keys[i].frame || frame == keys[i].frame + 2) {
            SDL_Event event;
            memset(&event, 0, sizeof event);
            event.type = frame == keys[i].frame ? SDL_KEYDOWN : SDL_KEYUP;
            event.key.windowID = window_id(window);
            event.key.state = event.type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
            event.key.keysym.scancode = (SDL_Scancode)keys[i].scan;
            event.key.keysym.sym = keys[i].sym;
            int rc = push(&event);
            fprintf(stderr, "INPUT SDL frame=%u type=%s scan=%d sym=%d rc=%d\n", frame,
                event.type == SDL_KEYDOWN ? "down" : "up", keys[i].scan, keys[i].sym, rc);
        }
    }
    if (real_swap) real_swap(window);
}
