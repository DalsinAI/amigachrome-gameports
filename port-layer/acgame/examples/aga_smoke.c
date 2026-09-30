#include "acgame/platform.h"

#include <stdio.h>
#include <string.h>

#define W 320
#define H 200

static void make_palette(acgame_indexed_surface *frame) {
    unsigned i;
    for (i = 0; i < 256; ++i) {
        frame->palette[i].r = (uint8_t)i;
        frame->palette[i].g = (uint8_t)((i * 5u) & 255u);
        frame->palette[i].b = (uint8_t)(255u - i);
    }
}

static void make_frame(acgame_indexed_surface *frame, unsigned phase) {
    unsigned x, y;
    for (y = 0; y < frame->height; ++y) {
        uint8_t *row = frame->pixels + (size_t)y * frame->pitch;
        for (x = 0; x < frame->width; ++x) {
            unsigned checker = ((x >> 4) ^ (y >> 4)) & 1u;
            row[x] = (uint8_t)((x + y + phase + checker * 48u) & 255u);
        }
    }
}

int main(void) {
    acgame_video_config config = {W, H, 50, ACGAME_VIDEO_INDEXED8};
    acgame_input_state input;
    acgame_indexed_surface *frame;
    unsigned phase = 0;

    if (acgame_platform_init(&config) != 0) {
        fputs("ACGame: failed to open AGA indexed screen\n", stderr);
        return 20;
    }
    frame = acgame_platform_begin_indexed_frame();
    if (!frame) {
        fputs("ACGame: no indexed frame available\n", stderr);
        acgame_platform_shutdown();
        return 21;
    }

    make_palette(frame);
    memset(&input, 0, sizeof(input));
    while (!input.quit_requested) {
        make_frame(frame, phase++);
        if (acgame_platform_present_indexed(frame) != 0) {
            fputs("ACGame: AGA present failed\n", stderr);
            acgame_platform_shutdown();
            return 22;
        }
        acgame_platform_poll(&input);
    }

    acgame_platform_shutdown();
    return 0;
}

[executed on device: daletop (557d2ffd-2bd4-42f8-8777-9a5024193e1e)]